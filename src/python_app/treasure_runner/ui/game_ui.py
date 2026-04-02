# game_ui.py - View layer for Treasure Runner A3
# Handles all curses rendering and user input
# No game logic here - just display and input handling
#curses examples from CIS2750 lectures 16and17 andthe  lab 5

import curses
import json
import os
from datetime import datetime, timezone
from ..models.game_engine import GameEngine
from ..models.exceptions import ImpassableError, GameError
from ..bindings.bindings import Direction


class GameUI:
    # The view class - handles everything the player sees and types
    # Talks to the GameEngine (controller) to get state and send moves

    #minimum terminal size needed to display the game
    MIN_HEIGHT = 24
    MIN_WIDTH = 60

    # colour pair ids - defined in _init_colors
    COLOR_DEFAULT = 1
    COLOR_PLAYER = 2
    COLOR_TREASURE = 3
    COLOR_WALL = 4
    COLOR_PORTAL = 5
    COLOR_PUSHABLE = 6
    COLOR_MSG_BAR = 7
    COLOR_STATUS = 8

    def __init__(self, stdscr, engine, profile_path):
        # stdscr - curses screen from wrapper
        # engine - the game engine controller
        # profile_path - where to load/save the player profile json
        self._stdscr = stdscr
        self._engine = engine
        self._profile_path = profile_path
        self._profile = {}
        self._message = "Welcome to Treasure Runner!"
        self._running = False
        self._won = False
        self._colors_enabled = False
        self._visited_rooms = set()

    def _init_colors(self):
        # set up colour pairs for the UI elements
        # only runs if the terminal supports color
        if not curses.has_colors():
            return
        curses.start_color()
        curses.use_default_colors()
        try:
            curses.init_pair(self.COLOR_DEFAULT, curses.COLOR_WHITE, -1)
            curses.init_pair(self.COLOR_PLAYER, curses.COLOR_GREEN, -1)
            curses.init_pair(self.COLOR_TREASURE, curses.COLOR_YELLOW, -1)
            curses.init_pair(self.COLOR_WALL, curses.COLOR_WHITE, -1)
            curses.init_pair(self.COLOR_PORTAL, curses.COLOR_CYAN, -1)
            curses.init_pair(self.COLOR_PUSHABLE, curses.COLOR_MAGENTA, -1)
            curses.init_pair(self.COLOR_MSG_BAR, curses.COLOR_WHITE, curses.COLOR_BLUE)
            curses.init_pair(self.COLOR_STATUS, curses.COLOR_BLACK, curses.COLOR_WHITE)
            self._colors_enabled = True
        except curses.error:
            # if color init fails just continue without color
            self._colors_enabled = False

    def _color(self, pair_id):
        # return the color pair attribute, or 0 if colors not available
        if self._colors_enabled:
            return curses.color_pair(pair_id)
        return 0

    def _char_color(self, char):
        # return the right color for a given room character
        # bold chars need A_BOLD combined with their color pair
        bold_map = {
            '@': self.COLOR_PLAYER,
            '$': self.COLOR_TREASURE,
            'x': self.COLOR_PORTAL,
            'O': self.COLOR_PORTAL,
            'o': self.COLOR_PORTAL,
        }
        plain_map = {
            '#': self.COLOR_WALL,
            '*': self.COLOR_PUSHABLE,
        }
        if char in bold_map:
            return self._color(bold_map[char]) | curses.A_BOLD
        return self._color(plain_map.get(char, self.COLOR_DEFAULT))

    def load_profile(self):
        # load the player profile from the json file
        # if the file doesnt exist, prompt for a name and create it
        if os.path.exists(self._profile_path):
            with open(self._profile_path, "r", encoding="utf-8") as profile_file:
                self._profile = json.load(profile_file)
        else:
            # profile missing - prompt for player name using curses
            name = self._prompt_for_name()
            self._profile = {
                "player_name": name,
                "games_played": 0,
                "max_treasure_collected": 0,
                "most_rooms_world_completed": 0,
                "timestamp_last_played": datetime.now(timezone.utc).strftime(
                    "%Y-%m-%dT%H:%M:%SZ"
                ),
            }
            # save the new profile right away
            self._save_profile()

    def _prompt_for_name(self):
        # show a simple prompt on screen asking for the player name
        # uses curses getstr to read input
        self._stdscr.clear()
        max_y, max_x = self._stdscr.getmaxyx()
        prompt = "No profile found! Enter your name: "
        y = max_y // 2
        x = max(0, (max_x - len(prompt)) // 2)
        self._stdscr.addstr(y, x, prompt)
        curses.echo()
        curses.curs_set(1)
        name_bytes = self._stdscr.getstr(y, x + len(prompt), 30)
        curses.noecho()
        curses.curs_set(0)
        name = name_bytes.decode("utf-8").strip()
        if not name:
            name = "Player"
        return name

    def _save_profile(self):
        # save the current profile back to the json file
        # always store in assets/ as per the spec
        os.makedirs(os.path.dirname(self._profile_path), exist_ok=True)
        with open(self._profile_path, "w", encoding="utf-8") as profile_file:
            json.dump(self._profile, profile_file, indent=4)

    def _update_profile_after_game(self):
        # update stats in the profile after the game ends
        player = self._engine.get_player()
        collected = player.get_collected_count()
        room_count = self._engine.get_room_count()
        self._profile["games_played"] = self._profile.get("games_played", 0) + 1
        if collected > self._profile.get("max_treasure_collected", 0):
            self._profile["max_treasure_collected"] = collected
        if room_count > self._profile.get("most_rooms_world_completed", 0):
            self._profile["most_rooms_world_completed"] = room_count
        self._profile["timestamp_last_played"] = datetime.now(timezone.utc).strftime(
            "%Y-%m-%dT%H:%M:%SZ"
        )
        self._save_profile()

    def _draw_lines(self, lines):
        # helper to draw a centered list of lines on the screen
        self._stdscr.clear()
        max_y, max_x = self._stdscr.getmaxyx()
        start_y = max(0, (max_y - len(lines)) // 2)
        for idx, line in enumerate(lines):
            row = start_y + idx
            if row >= max_y - 1:
                break
            col = max(0, (max_x - len(line)) // 2)
            self._stdscr.addstr(row, col, line[:max_x - 1])
        self._stdscr.refresh()

    def _build_splash_lines(self):
        # build the startup splash screen lines from profile data
        name = self._profile.get("player_name", "Player")
        games = self._profile.get("games_played", 0)
        max_gold = self._profile.get("max_treasure_collected", 0)
        last_played = self._profile.get("timestamp_last_played", "Never")
        return [
            "================================",
            "   TREASURE RUNNER - A3",
            "================================",
            "",
            f"  Player:       {name}",
            f"  Games Played: {games}",
            f"  Most Gold:    {max_gold}",
            f"  Last Played:  {last_played}",
            "",
            "  Controls:",
            "  Arrow keys / WASD - move",
            "  > - use portal",
            "  r - reset game",
            "  q - quit",
            "",
            "  Press any key to start...",
        ]

    def _build_quit_lines(self):
        # build the quit screen lines from profile and current run data
        player = self._engine.get_player()
        collected = player.get_collected_count()
        name = self._profile.get("player_name", "Player")
        games = self._profile.get("games_played", 0)
        max_gold = self._profile.get("max_treasure_collected", 0)
        return [
            "================================",
            "   GAME OVER - TREASURE RUNNER",
            "================================",
            "",
            f"  Player:          {name}",
            f"  Gold This Run:   {collected}",
            f"  Games Played:    {games}",
            f"  Best Gold Ever:  {max_gold}",
            "",
            "  Thanks for playing!",
            "",
            "  Press any key to exit...",
        ]

    def show_splash(self):
        # startup splash screen - shows profile info and waits for a key
        self._draw_lines(self._build_splash_lines())
        self._stdscr.getch()

    def show_quit_screen(self):
        # quit/end screen - shows updated profile stats and waits for a key
        self._draw_lines(self._build_quit_lines())
        self._stdscr.getch()

    def _check_terminal_size(self):
        # raise an error if the terminal is too small to show the game
        max_y, max_x = self._stdscr.getmaxyx()
        if max_y < self.MIN_HEIGHT or max_x < self.MIN_WIDTH:
            raise RuntimeError(
                f"Terminal too small: need {self.MIN_WIDTH}x{self.MIN_HEIGHT}, "
                f"got {max_x}x{max_y}"
            )

    def _draw_message_bar(self):
        # draw the message bar at the top with blue background
        _max_y, max_x = self._stdscr.getmaxyx()
        msg = self._message[:max_x - 1]
        self._stdscr.addstr(
            0, 0, msg.ljust(max_x - 1), self._color(self.COLOR_MSG_BAR)
        )

    def _draw_room(self, room_row):
        # draw the current room with colors for each element type
        max_y, max_x = self._stdscr.getmaxyx()
        room_id = self._engine.get_player().get_room()
        self._stdscr.addstr(
            room_row, 0, f"Room: {room_id}"[:max_x - 1], self._color(self.COLOR_DEFAULT)
        )
        room_row += 1
        rendered = self._engine.render_current_room()
        for line in rendered.splitlines():
            if room_row >= max_y - 4:
                break
            col = 0
            for char in line[:max_x - 1]:
                if col >= max_x - 1:
                    break
                self._stdscr.addch(room_row, col, char, self._char_color(char))
                col += 1
            room_row += 1
        return room_row

    def _draw_legend(self, start_col):
        # draw the game elements legend on the right side with colors
        max_y, max_x = self._stdscr.getmaxyx()
        self._stdscr.addstr(
            2, start_col, "Game Elements:"[:max_x - start_col - 1],
            self._color(self.COLOR_DEFAULT) | curses.A_BOLD
        )
        legend_items = [
            ("@ - player", self.COLOR_PLAYER),
            ("# - wall", self.COLOR_WALL),
            ("$ - gold", self.COLOR_TREASURE),
            ("x - exit/portal", self.COLOR_PORTAL),
            ("* - pushable", self.COLOR_PUSHABLE),
        ]
        row = 3
        for text, color_id in legend_items:
            if row >= max_y - 4 or start_col + len(text) >= max_x:
                break
            self._stdscr.addstr(
                row, start_col, text[:max_x - start_col - 1],
                self._color(color_id)
            )
            row += 1

    def _draw_controls(self, start_col, start_row):
        # draw the controls list below the legend
        max_y, max_x = self._stdscr.getmaxyx()
        controls = [
            "Controls:",
            "Arrows/WASD-move",
            "> - portal",
            "r - reset",
            "q - quit",
        ]
        row = start_row
        for line in controls:
            if row >= max_y - 4 or start_col + len(line) >= max_x:
                break
            self._stdscr.addstr(row, start_col, line[:max_x - start_col - 1])
            row += 1

    def _draw_status_bar(self):
        # draw the player status bar near the bottom of the screen
        max_y, max_x = self._stdscr.getmaxyx()
        player = self._engine.get_player()
        collected = player.get_collected_count()
        room_id = player.get_room()
        total = self._engine.get_total_treasure_count()
        status = (
            f" {self._profile.get('player_name', 'Player')} | "
            f"Gold: {collected}/{total} | Room: {room_id}"
        )
        self._stdscr.addstr(
            max_y - 3, 0,
            status[:max_x - 1].ljust(max_x - 1),
            self._color(self.COLOR_STATUS)
        )
        title = "Treasure Runner A3  raaif@uoguelph.ca"
        self._stdscr.addstr(max_y - 2, 0, title[:max_x - 1])

    def draw(self):
        # main draw method - clears and redraws the full screen
        self._stdscr.clear()
        self._check_terminal_size()
        self._draw_message_bar()
        _max_y, max_x = self._stdscr.getmaxyx()
        self._draw_room(1)
        legend_col = max_x - 20
        if legend_col > 0:
            self._draw_legend(legend_col)
            self._draw_controls(legend_col, 9)
            self._draw_minimap(15, legend_col)
        self._draw_status_bar()
        self._stdscr.refresh()

    def _try_portal(self):
        # try all 4 directions to find and use a portal
        # player_before is set once outside the loop so we can detect room changes
        player_before = self._engine.get_player().get_room()
        reverse = {
            Direction.NORTH: Direction.SOUTH,
            Direction.SOUTH: Direction.NORTH,
            Direction.EAST: Direction.WEST,
            Direction.WEST: Direction.EAST,
        }
        for direction in [Direction.NORTH, Direction.SOUTH,
                          Direction.EAST, Direction.WEST]:
            try:
                self._engine.move_player(direction)
                if self._engine.get_player().get_room() != player_before:
                    self._message = "You went through a portal!"
                    return
                # moved to a floor tile, not a portal - undo the move
                self._engine.move_player(reverse[direction])
            except (ImpassableError, GameError):
                continue
        self._message = "No portal here to use."

    def _handle_key(self, key):
        # translate a keypress into a game action
        # returns False if the game should quit, True otherwise
        move_map = {
            curses.KEY_UP: Direction.NORTH,
            curses.KEY_DOWN: Direction.SOUTH,
            curses.KEY_LEFT: Direction.WEST,
            curses.KEY_RIGHT: Direction.EAST,
            ord('w'): Direction.NORTH,
            ord('s'): Direction.SOUTH,
            ord('a'): Direction.WEST,
            ord('d'): Direction.EAST,
        }
        if key == ord('q'):
            self._running = False
            return False
        if key == ord('r'):
            self._engine.reset()
            self._message = "Game reset to the beginning!"
            return True
        if key == ord('>'):
            self._try_portal()
            # THIS WILL TRACK THE VISITED ROOMS IN THE _HANDLE_KEY
            self._visited_rooms.add(self._engine.get_player().get_room())
            return True
        if key in move_map:
            direction = move_map[key]
            try:
                self._engine.move_player(direction)
                self._visited_rooms.add(self._engine.get_player().get_room())
                collected = self._engine.get_player().get_collected_count()
                total = self._engine.get_total_treasure_count()
                if collected >= total > 0:
                    self._won = True
                    self._running = False
                else:
                    self._message = f"You moved. Gold: {collected}/{total}"
            except ImpassableError:
                self._message = "You can't go that way!"
            except GameError as game_err:
                self._message = f"Error: {game_err}"
        return True
    def _show_victory_screen(self):
        # victory screen when player collects all treasure
        player = self._engine.get_player()
        collected = player.get_collected_count()
        name = self._profile.get("player_name", "Player")
        lines = [
            "================================",
            "   YOU WIN! TREASURE RUNNER",
            "================================",
            "",
            f"  Player:       {name}",
            f"  Gold:         {collected}/{collected}",
            "",
            "  You collected ALL the treasure!",
            "  Amazing work!",
            "",
            "  Press any key to exit...",
        ]
        self._draw_lines(lines)
        self._stdscr.getch()

    #linter was giving issues, so needed to break this out into a separate method to avoid too many local variables in _draw_minimap
    def _draw_minimap_node(self, row, col, room_id, current_room, conn_str, max_x):
        # draw a single room node with pre-computed connection string
        if room_id == current_room:
            symbol, attr = f"[{room_id}]", self._color(self.COLOR_PLAYER) | curses.A_BOLD
        elif room_id in self._visited_rooms:
            symbol, attr = f"({room_id})", self._color(self.COLOR_TREASURE)
        else:
            symbol, attr = f" {room_id} ", self._color(self.COLOR_DEFAULT)
        line = f"{symbol}{conn_str}"
        if col + len(line) < max_x - 1:
            self._stdscr.addstr(row, col, line, attr)

    def _draw_minimap(self, start_row, start_col):
        # draw minimap showing rooms as nodes with connections
        # current room highlighted, visited rooms colour-coded
        try:
            matrix, ids, num_rooms = self._engine.get_adjacency_matrix()
        except Exception:
            return
        if num_rooms == 0:
            return
        self._draw_minimap_header(start_row, start_col)
        self._draw_minimap_nodes(start_row, start_col, matrix, ids, num_rooms)

    def _draw_minimap_nodes(self, start_row, start_col, matrix, ids, num_rooms):
        # draw each room node with connection arrows
        for i in range(num_rooms):
            row = start_row + 2 + i
            if row >= self._stdscr.getmaxyx()[0] - 4:
                break
            conn = "->(" + ",".join(
                str(ids[j]) for j in range(num_rooms) if matrix[i * num_rooms + j] == 1
            ) + ")"
            self._draw_minimap_node(
                row, start_col, ids[i],
                self._engine.get_player().get_room(),
                conn, self._stdscr.getmaxyx()[1]
            )

    def _draw_minimap_header(self, start_row, start_col):
        # draw the map title and legend
        max_x = self._stdscr.getmaxyx()[1]
        self._stdscr.addstr(
            start_row, start_col,
            "Map:"[:max_x - start_col - 1],
            self._color(self.COLOR_DEFAULT) | curses.A_BOLD
        )
        if start_col + 18 < max_x - 1:
            self._stdscr.addstr(start_row + 1, start_col,
                                "[cur] (vis) unvis",
                                self._color(self.COLOR_DEFAULT))

    def run(self):
        # main game loop - sets up curses and runs until the player quits
        curses.curs_set(0)
        self._stdscr.keypad(True)
        self._init_colors()
        self.load_profile()
        self.show_splash()
        self._running = True
        self._visited_rooms.add(self._engine.get_player().get_room())
        while self._running:
            self.draw()
            key = self._stdscr.getch()
            self._handle_key(key)
        # game ended - update profile and show quit screen
        self._update_profile_after_game()
        if self._won:
            self._show_victory_screen()
        else:
            self.show_quit_screen()
