# logger_config.py
import logging
import colorlog

def get_logger(name='main', level=logging.INFO):
    """
    Возвращает настроенный логгер с заданным именем.
    """
    logger = logging.getLogger(name)
    logger.setLevel(level)

    if not logger.handlers:  # Чтобы не добавлять обработчики повторно при многократном вызове
        console_handler = colorlog.StreamHandler()
        console_handler.setLevel(logging.DEBUG)

        formatter = colorlog.ColoredFormatter(
            '%(log_color)s%(asctime)s - %(name)s - %(levelname)s - %(message)s',
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
