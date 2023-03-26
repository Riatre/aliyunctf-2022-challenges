from starlette_context import context
import structlog
import structlog.processors
import structlog.stdlib
import logging
import sys


def setup_logging():
    def add_app_context(logger, method_name, event_dict):
        if context.exists():
            event_dict.update(context.data)
        return event_dict

    if sys.stderr.isatty():
        renderer = [structlog.dev.ConsoleRenderer()]
    else:
        renderer = [
            structlog.processors.dict_tracebacks,
            structlog.processors.JSONRenderer(),
        ]
    structlog.configure(
        processors=[
            structlog.contextvars.merge_contextvars,
            add_app_context,
            structlog.processors.add_log_level,
            structlog.processors.StackInfoRenderer(),
            structlog.dev.set_exc_info,
            structlog.processors.TimeStamper(),
        ]
        + renderer,
        wrapper_class=structlog.make_filtering_bound_logger(logging.NOTSET),
        context_class=dict,
        logger_factory=structlog.PrintLoggerFactory(),
        cache_logger_on_first_use=True,
    )
