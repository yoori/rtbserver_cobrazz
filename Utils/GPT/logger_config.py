# logger_config.py
import logging
import colorlog


def get_logger(name='main', level=logging.INFO):
    """
    Returns the configured logger with the specified name.
    """
    logger = logging.getLogger(name)
    logger.setLevel(level)

    if not logger.handlers:
        console_handler = colorlog.StreamHandler()
        console_handler.setLevel(logging.DEBUG)

        formatter = colorlog.ColoredFormatter(
            '%(log_color)s%(asctime)s.%(msecs)03d - %(name)s:%(lineno)d - %(levelname)s - %(message)s',
            datefmt='%Y-%m-%d %H:%M:%S',
            log_colors={
                'DEBUG': 'cyan',
                'INFO': 'green',
                'WARNING': 'yellow',
                'ERROR': 'red',
                'CRITICAL': 'bold_red',
            }
        )
        console_handler.setFormatter(formatter)
        logger.addHandler(console_handler)

    return logger
