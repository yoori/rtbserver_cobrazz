import json
import pathlib


class Config:
  def __init__(self):
    self.clickhouse_conn = ''
    self.postgres_conn = None
    self.pid_file = None
    self.workspace_root = None
    self.generate_period = 3600.0
    self.selection_chunk_rows = 7000000
    self.main_chunk_rows = 10000000
    self.validation_set_rows = 200000
    self.selection_validation_sets = 3
    self.training_validation_sets = 3
    self.final_test_sets = 3
    self.selection_fit_steps = 10
    self.training_fit_steps = 30
    self.fit_iterations = 10
    self.selection_patience = 3
    self.training_patience = 5
    self.campaign_model_activity_period = 14 * 24 * 60 * 60
    self.min_campaign_model_imps = 100000
    self.data_delay = None
    self.algorithm_id = 'catboost'

  def init_json(self, config_json):
    def required_string(name):
      value = config_json.get(name)
      if not isinstance(value, str) or not value:
        raise ValueError("Configuration value '" + name + "' is required")
      return value

    self.pid_file = required_string('pid_file')
    self.workspace_root = required_string('workspace_root')
    self.clickhouse_conn = config_json.get('clickhouse_conn', '')
    self.postgres_conn = required_string('postgres_conn')
    self.algorithm_id = config_json.get('algorithm_id', 'catboost')
    self.generate_period = float(config_json.get('generate_period', 3600.0))
    self.selection_chunk_rows = int(
      config_json.get('selection_chunk_rows', 7000000))
    self.main_chunk_rows = int(
      config_json.get('main_chunk_rows', 10000000))
    self.validation_set_rows = int(
      config_json.get('validation_set_rows', 200000))
    self.selection_validation_sets = int(
      config_json.get('selection_validation_sets', 3))
    self.training_validation_sets = int(
      config_json.get('training_validation_sets', 3))
    self.final_test_sets = int(config_json.get('final_test_sets', 3))
    self.selection_fit_steps = int(
      config_json.get('selection_fit_steps', 10))
    self.training_fit_steps = int(
      config_json.get('training_fit_steps', 30))
    self.fit_iterations = int(config_json.get('fit_iterations', 10))
    self.selection_patience = int(
      config_json.get('selection_patience', 3))
    self.training_patience = int(config_json.get('training_patience', 5))
    self.campaign_model_activity_period = int(config_json.get(
      'campaign_model_activity_period',
      14 * 24 * 60 * 60))
    self.min_campaign_model_imps = int(config_json.get(
      'min_campaign_model_imps',
      100000))
    try:
      self.data_delay = int(config_json['data_delay'])
    except (KeyError, TypeError, ValueError):
      raise ValueError(
        "Configuration value 'data_delay' must be a positive integer")

    if not isinstance(self.clickhouse_conn, str):
      raise ValueError("Configuration value 'clickhouse_conn' must be a string")
    if not isinstance(self.algorithm_id, str) or not self.algorithm_id:
      raise ValueError("Configuration value 'algorithm_id' must be non-empty")
    if self.generate_period <= 0:
      raise ValueError('generate_period must be positive')
    if self.selection_chunk_rows <= 0:
      raise ValueError('selection_chunk_rows must be positive')
    if self.main_chunk_rows <= 0:
      raise ValueError('main_chunk_rows must be positive')
    if self.validation_set_rows <= 0:
      raise ValueError('validation_set_rows must be positive')
    for name in (
        'selection_validation_sets',
        'training_validation_sets',
        'final_test_sets',
        'selection_fit_steps',
        'training_fit_steps',
        'fit_iterations',
        'selection_patience',
        'training_patience',
        'campaign_model_activity_period',
        'min_campaign_model_imps'):
      if getattr(self, name) <= 0:
        raise ValueError(name + ' must be positive')
    if self.data_delay <= 0:
      raise ValueError('data_delay must be positive')

  def model_root(self):
    return pathlib.Path(
      self.workspace_root) / 'log' / 'Predictor' / 'CTRConfig'


def load_config(file_name):
  with open(file_name, 'r') as file:
    config_json = json.load(file)
  config = Config()
  config.init_json(config_json)
  return config
