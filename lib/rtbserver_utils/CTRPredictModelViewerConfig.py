import json


class Config:
  def __init__(self):
    self.pid_file = None
    self.model_root = None
    self.web_host = '0.0.0.0'
    self.web_port = None

  def init_json(self, config_json):
    def required_string(name):
      value = config_json.get(name)
      if not isinstance(value, str) or not value:
        raise ValueError("Configuration value '" + name + "' is required")
      return value

    self.pid_file = required_string('pid_file')
    self.model_root = required_string('model_root')

    web_server = config_json.get('web_server')
    if not isinstance(web_server, dict):
      raise ValueError("Configuration value 'web_server' is required")
    self.web_host = web_server.get('host', '0.0.0.0')
    try:
      self.web_port = int(web_server['port'])
    except (KeyError, TypeError, ValueError):
      raise ValueError(
        "Configuration value 'web_server.port' must be an integer")

    if not isinstance(self.web_host, str) or not self.web_host:
      raise ValueError("Configuration value 'web_server.host' must be non-empty")
    if self.web_port <= 0 or self.web_port > 65535:
      raise ValueError('web_server.port must be in range 1..65535')


def load_config(file_name):
  with open(file_name, 'r') as file:
    config_json = json.load(file)
  config = Config()
  config.init_json(config_json)
  return config
