import dataclasses
import json
import pathlib


@dataclasses.dataclass(frozen=True)
class SegmentModelViewerConfig:
  pid_file: pathlib.Path
  model_root: pathlib.Path
  web_host: str
  web_port: int
  url_path: str

  @classmethod
  def from_json(cls, config_file):
    config_file = pathlib.Path(config_file)
    with config_file.open(encoding='utf-8') as input_file:
      value = json.load(input_file)
    web_server = value.get('web_server')
    if not isinstance(web_server, dict):
      raise ValueError('web_server is required')
    pid_file = value.get('pid_file')
    model_root = value.get('model_root')
    web_host = web_server.get('host')
    web_port = web_server.get('port')
    url_path = value.get('url_path', '/')
    if not isinstance(pid_file, str) or not pid_file:
      raise ValueError('pid_file is required')
    if not isinstance(model_root, str) or not model_root:
      raise ValueError('model_root is required')
    if not isinstance(web_host, str) or not web_host:
      raise ValueError('web_server.host is required')
    if not isinstance(web_port, int) or isinstance(web_port, bool):
      raise ValueError('web_server.port must be an integer')
    if not 1 <= web_port <= 65535:
      raise ValueError('web_server.port must be between 1 and 65535')
    if not isinstance(url_path, str):
      raise ValueError('url_path must be a string')
    return cls(
      pathlib.Path(pid_file),
      pathlib.Path(model_root),
      web_host,
      web_port,
      url_path)
