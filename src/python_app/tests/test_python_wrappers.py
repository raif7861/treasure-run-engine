import os
import unittest

from treasure_runner.bindings import Direction
from treasure_runner.models.game_engine import GameEngine
from treasure_runner.models.exceptions import ImpassableError


class TestTreasureRunnerPython(unittest.TestCase):
    """Python unit tests for the Treasure Runner wrapper layer."""

    def setUp(self):
        """
        Build the config path once for each test.

        This points to the starter world file used throughout A2.
        """
        here = os.path.abspath(os.path.dirname(__file__))
        self.config_path = os.path.abspath(
            os.path.join(here, "..", "..", "assets", "starter.ini")
        )

    def test_engine_create_and_room_count(self):
        """Engine should create successfully and report at least one room."""
        with GameEngine(self.config_path) as engine:
            count = engine.get_room_count()
            self.assertGreaterEqual(count, 1)

    def test_engine_get_room_dimensions(self):
        """Current room dimensions should both be positive."""
        with GameEngine(self.config_path) as engine:
            width, height = engine.get_room_dimensions()
            self.assertGreater(width, 0)
            self.assertGreater(height, 0)

    def test_engine_get_room_ids(self):
        """Room IDs should come back as a Python list and include room 0."""
        with GameEngine(self.config_path) as engine:
            room_ids = engine.get_room_ids()
            self.assertIsInstance(room_ids, list)
            self.assertGreaterEqual(len(room_ids), 1)
            self.assertIn(0, room_ids)

    def test_get_player(self):
        """Game engine should return a valid Player wrapper."""
        with GameEngine(self.config_path) as engine:
            player = engine.get_player()
            self.assertIsNotNone(player)

            room_id = player.get_room_id()
            pos = player.get_position()

            self.assertIsInstance(room_id, int)
            self.assertIsInstance(pos, tuple)
            self.assertEqual(len(pos), 2)

    def test_render_current_room_returns_string(self):
        """Rendering the current room should return a non-empty string."""
        with GameEngine(self.config_path) as engine:
            rendered = engine.render_current_room()
            self.assertIsInstance(rendered, str)
            self.assertGreater(len(rendered), 0)

    def test_render_room_returns_string(self):
        """Rendering a room by ID should return a non-empty string."""
        with GameEngine(self.config_path) as engine:
            rendered = engine.render_room(0)
            self.assertIsInstance(rendered, str)
            self.assertGreater(len(rendered), 0)

    def test_move_player_changes_position(self):
        """
        A valid move should change the player's position.

        From the starter spawn, moving EAST is legal.
        """
        with GameEngine(self.config_path) as engine:
            player = engine.get_player()
            start_room = player.get_room_id()
            start_pos = player.get_position()

            engine.move_player(Direction.EAST)

            player = engine.get_player()
            end_room = player.get_room_id()
            end_pos = player.get_position()

            self.assertEqual(start_room, end_room)
            self.assertNotEqual(start_pos, end_pos)

    def test_blocked_move_raises_impassable(self):
        """
        Repeatedly moving SOUTH from the entry path should eventually hit a wall.

        That should raise ImpassableError.
        """
        with GameEngine(self.config_path) as engine:
            engine.move_player(Direction.EAST)
            engine.move_player(Direction.SOUTH)
            engine.move_player(Direction.SOUTH)
            engine.move_player(Direction.SOUTH)
            engine.move_player(Direction.SOUTH)
            engine.move_player(Direction.SOUTH)
            engine.move_player(Direction.SOUTH)

            with self.assertRaises(ImpassableError):
                engine.move_player(Direction.SOUTH)

    def test_player_collected_count_updates(self):
        """
        Move along a known path to collect one treasure.

        After collection, player's collected count should be 1.
        """
        with GameEngine(self.config_path) as engine:
            player = engine.get_player()
            self.assertEqual(player.get_collected_count(), 0)

            engine.move_player(Direction.EAST)
            engine.move_player(Direction.SOUTH)
            engine.move_player(Direction.SOUTH)
            engine.move_player(Direction.SOUTH)
            engine.move_player(Direction.SOUTH)
            engine.move_player(Direction.SOUTH)
            engine.move_player(Direction.SOUTH)

            engine.move_player(Direction.NORTH)
            engine.move_player(Direction.NORTH)
            engine.move_player(Direction.NORTH)
            engine.move_player(Direction.NORTH)
            engine.move_player(Direction.NORTH)
            engine.move_player(Direction.NORTH)
            engine.move_player(Direction.NORTH)
            engine.move_player(Direction.NORTH)
            engine.move_player(Direction.NORTH)

            engine.move_player(Direction.EAST)

            player = engine.get_player()
            self.assertEqual(player.get_collected_count(), 1)

    def test_player_get_collected_treasures(self):
        """
        After collecting one treasure, the Python wrapper should return
        a list containing that treasure's info.
        """
        with GameEngine(self.config_path) as engine:
            engine.move_player(Direction.EAST)
            engine.move_player(Direction.SOUTH)
            engine.move_player(Direction.SOUTH)
            engine.move_player(Direction.SOUTH)
            engine.move_player(Direction.SOUTH)
            engine.move_player(Direction.SOUTH)
            engine.move_player(Direction.SOUTH)

            engine.move_player(Direction.NORTH)
            engine.move_player(Direction.NORTH)
            engine.move_player(Direction.NORTH)
            engine.move_player(Direction.NORTH)
            engine.move_player(Direction.NORTH)
            engine.move_player(Direction.NORTH)
            engine.move_player(Direction.NORTH)
            engine.move_player(Direction.NORTH)
            engine.move_player(Direction.NORTH)

            engine.move_player(Direction.EAST)

            player = engine.get_player()
            treasures = player.get_collected_treasures()

            self.assertIsInstance(treasures, list)
            self.assertEqual(len(treasures), 1)
            self.assertIn("id", treasures[0])
            self.assertIn("name", treasures[0])
            self.assertIn("collected", treasures[0])
            self.assertTrue(treasures[0]["collected"])

    def test_reset_clears_collected_treasures(self):
        """
        After collecting a treasure, reset should:
        - clear player collection
        - return player to starting room/position
        """
        with GameEngine(self.config_path) as engine:
            engine.move_player(Direction.EAST)
            engine.move_player(Direction.SOUTH)
            engine.move_player(Direction.SOUTH)
            engine.move_player(Direction.SOUTH)
            engine.move_player(Direction.SOUTH)
            engine.move_player(Direction.SOUTH)
            engine.move_player(Direction.SOUTH)

            engine.move_player(Direction.NORTH)
            engine.move_player(Direction.NORTH)
            engine.move_player(Direction.NORTH)
            engine.move_player(Direction.NORTH)
            engine.move_player(Direction.NORTH)
            engine.move_player(Direction.NORTH)
            engine.move_player(Direction.NORTH)
            engine.move_player(Direction.NORTH)
            engine.move_player(Direction.NORTH)

            engine.move_player(Direction.EAST)

            player = engine.get_player()
            self.assertEqual(player.get_collected_count(), 1)

            engine.reset()

            player = engine.get_player()
            self.assertEqual(player.get_collected_count(), 0)
            self.assertEqual(player.get_room_id(), 0)
            self.assertEqual(player.get_position(), (0, 5))


if __name__ == "__main__":
    unittest.main()
