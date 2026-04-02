"""
Exceptions for game models layer.

These are raised when the C layer returns error codes,
allowing Python code to use standard exception handling.
"""

from ..bindings import Status


class GameError(Exception):
    """Base exception for game errors."""


class GameEngineError(GameError):
    """Raised when game engine operation fails."""


class InvalidArgumentError(GameEngineError):
    """Raised when invalid argument passed to game engine."""


class OutOfBoundsError(GameEngineError):
    """Raised when operation is out of bounds."""


class ImpassableError(GameEngineError):
    """Raised when movement blocked by obstacle."""


class NoSuchRoomError(GameEngineError):
    """Raised when referenced room doesn't exist."""


class NoPortalError(GameEngineError):
    """Raised when portal not found or not accessible."""


class InternalError(GameEngineError):
    """Raised for internal engine logic errors."""


class StatusError(GameError):
    """Base exception for status errors."""


class StatusInvalidArgumentError(StatusError):
    """Raised when invalid argument passed."""


class StatusNullPointerError(StatusError):
    """Raised when null pointer encountered."""


class StatusNoMemoryError(StatusError):
    """Raised when memory allocation fails."""


class StatusBoundsExceededError(StatusError):
    """Raised when bounds exceeded."""


class StatusNotFoundError(StatusError):
    """Raised when item not found."""


class StatusDuplicateError(StatusError):
    """Raised when duplicate item."""


class StatusInvalidStateError(StatusError):
    """Raised when invalid state."""


class StatusImpassableError(StatusError):
    """Raised when impassable."""


class StatusInternalError(StatusError):
    """Raised for internal errors."""


def status_to_exception(status, message=""):
    """Convert a GameEngineStatus to appropriate exception."""
    error_map = {
        Status.INVALID_ARGUMENT: InvalidArgumentError,
        Status.BOUNDS_EXCEEDED: OutOfBoundsError,
        Status.ROOM_IMPASSABLE: ImpassableError,
        Status.ROOM_NO_PORTAL: NoPortalError,
        Status.ROOM_NOT_FOUND: NoSuchRoomError,
        Status.GE_NO_SUCH_ROOM: NoSuchRoomError,
        Status.INTERNAL_ERROR: InternalError,
        Status.NULL_POINTER: InternalError,
        Status.NO_MEMORY: InternalError,
        Status.WL_ERR_CONFIG: GameEngineError,
        Status.WL_ERR_DATAGEN: GameEngineError,
    }

    exc_class = error_map.get(status, GameEngineError)
    return exc_class(message)


def status_to_status_exception(status, message=""):
    """Convert a Status to appropriate exception."""
    error_map = {
        Status.INVALID_ARGUMENT: StatusInvalidArgumentError,
        Status.NULL_POINTER: StatusNullPointerError,
        Status.NO_MEMORY: StatusNoMemoryError,
        Status.BOUNDS_EXCEEDED: StatusBoundsExceededError,
        Status.ROOM_IMPASSABLE: StatusImpassableError,
        Status.ROOM_NO_PORTAL: StatusNotFoundError,
        Status.ROOM_NOT_FOUND: StatusNotFoundError,
        Status.GE_NO_SUCH_ROOM: StatusNotFoundError,
        Status.INTERNAL_ERROR: StatusInternalError,
        Status.WL_ERR_CONFIG: StatusInvalidStateError,
        Status.WL_ERR_DATAGEN: StatusInvalidStateError,
    }

    exc_class = error_map.get(status, StatusError)
    return exc_class(message)
