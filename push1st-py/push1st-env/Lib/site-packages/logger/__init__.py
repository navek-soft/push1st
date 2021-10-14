import logging
import logging.config
import os
import sys
import types

logging.getLogger('paramiko').setLevel(logging.WARNING)
logging.getLogger('requests').setLevel(logging.WARNING)

config_file = os.path.join(os.path.dirname(__file__), 'logging.conf')

logging.config.fileConfig(config_file, disable_existing_loggers=False)
logger = logging.getLogger()


def error(self, msg, *args, **kwargs):
    self.error(msg, *args, **kwargs)
    sys.exit(1)

logger.interrupt = types.MethodType(error, logger)
logger.info = logger.info
logger.warn = logger.warn
