# logger_config.py
import logging
import colorlog


def get_logger(name='main', level="INFO"):
    """
    Returns the configured logger with the specified name.
    """
    level = level.upper()
    if level == "DEBUG":
        logLevel = logging.DEBUG
    elif level == "INFO":
        logLevel=logging.INFO
    elif level == "WARNING":
        logLevel=logging.WARNING
    elif level == "ERROR":
        logLevel=logging.ERROR
    elif level == "CRITICAL":
        logLevel=logging.CRITICAL
    else:
        raise ValueError(f"Unknown log level: {level}")

    logger = logging.getLogger(name)
    logger.setLevel(logLevel)

    if not logger.handlers:
        console_handler = colorlog.StreamHandler()
        console_handler.setLevel(logging.DEBUG) # Set minimum level for console output(
                                                # meaning all messages will be shown higher than DEBUG)

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
