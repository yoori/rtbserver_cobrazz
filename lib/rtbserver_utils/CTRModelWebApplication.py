import decimal
import html
import json
import urllib.parse

from fastapi import FastAPI, HTTPException, Query
from fastapi.responses import FileResponse, HTMLResponse, Response

from rtbserver_utils.CTRModelRepository import ModelNotFound


def json_dumps(value):
  if isinstance(value, decimal.Decimal):
    return format(value, 'f')
  if isinstance(value, dict):
    return '{' + ','.join(
      json.dumps(key, ensure_ascii=False) + ':' + json_dumps(item)
      for key, item in value.items()
    ) + '}'
  if isinstance(value, (list, tuple)):
    return '[' + ','.join(json_dumps(item) for item in value) + ']'
  return json.dumps(value, ensure_ascii=False, allow_nan=False)


class DecimalJSONResponse(Response):
  media_type = 'application/json'

  def render(self, content):
    return json_dumps(content).encode('utf-8')


def html_text(value):
  return html.escape(str(value), quote=True)


def decimal_text(value):
  try:
    return format(decimal.Decimal(str(value)), 'f')
  except decimal.InvalidOperation:
    return str(value)


def metric_decimal_text(value):
  try:
    number = decimal.Decimal(str(value))
    if not number.is_finite():
      return str(value)
    return format(
      number.quantize(
        decimal.Decimal('0.000001'),
        rounding=decimal.ROUND_DOWN),
      'f')
  except decimal.InvalidOperation:
    return str(value)


def feature_importance_item(item):
  if not isinstance(item, dict):
    return {
      'score': decimal.Decimal(0),
      'score_text': '0',
      'feature': str(item),
      'name': '',
      'yes_share_text': '-',
      'yes_ctr_text': '-',
      'no_ctr_text': '-',
    }

  if 'score' in item and 'feature' in item:
    score = item['score']
    feature = item['feature']
    name = item.get('name', '')
  elif len(item) == 1:
    score, feature = next(iter(item.items()))
    name = ''
  else:
    score = 0
    feature = json_dumps(item)
    name = ''

  try:
    numeric_score = decimal.Decimal(str(score))
  except decimal.InvalidOperation:
    numeric_score = decimal.Decimal(0)
  if not numeric_score.is_finite():
    numeric_score = decimal.Decimal(0)
  return {
    'score': numeric_score,
    'score_text': decimal_text(score),
    'feature': str(feature),
    'name': '' if name is None else str(name),
    'yes_share_text': (
      metric_decimal_text(item['yes_share'])
      if 'yes_share' in item else '-'),
    'yes_ctr_text': (
      metric_decimal_text(item['yes_ctr'])
      if 'yes_ctr' in item else '-'),
    'no_ctr_text': (
      metric_decimal_text(item['no_ctr'])
      if 'no_ctr' in item else '-'),
  }


def render_model_list(models, selected_model_id):
  if not models:
    return '<p class="empty-list">No models</p>'

  items = []
  for model in models:
    model_id = model['id']
    selected = model_id == selected_model_id
    class_name = 'model-link selected' if selected else 'model-link'
    current = ' aria-current="page"' if selected else ''
    url = '/?model=' + urllib.parse.quote(model_id, safe='')
    if model.get('status') in ('in_progress', 'interrupted'):
      status = model['status']
      status_label = 'In progress' if status == 'in_progress' else 'Interrupted'
      details = (
        '<span class="model-status ' + status + '">' + status_label + '</span>'
        '<span>Started ' + html_text(model.get('train_start') or '-') + '</span>'
        '<span>' + str(model.get('campaign_models_count', 0)) +
        ' campaign models</span>')
    else:
      components_count = model.get('components_count', 0)
      details = (
        '<span>' + html_text(model.get('algorithm_id') or 'unknown') + '</span>'
        '<span>' + (
          str(components_count) + ' components' if components_count else
          str(model.get('features_importance_count', 0)) + ' features') +
        '</span>')
    items.append(
      '<li><a class="' + class_name + '" href="' + url + '"' + current + '>'
      '<strong>' + html_text(model_id) + '</strong>' + details + '</a></li>')
  return '<ol class="model-list">' + ''.join(items) + '</ol>'


def render_feature_groups(feature_groups):
  if not feature_groups:
    return '<span class="muted">None</span>'
  return '<span class="feature-groups">' + ', '.join(
    html_text(' + '.join(str(value) for value in group))
    for group in feature_groups
  ) + '</span>'


def render_logloss_history(traits):
  history = []
  for item in traits.get('logloss_history', []):
    if not isinstance(item, dict):
      continue
    try:
      step = int(item['step'])
      train = decimal.Decimal(str(item['train']))
      test = decimal.Decimal(str(item['test']))
    except (KeyError, TypeError, ValueError, decimal.InvalidOperation):
      continue
    if train.is_finite() and test.is_finite():
      history.append({'step': step, 'train': train, 'test': test})

  if not history:
    return ''

  width = decimal.Decimal(900)
  height = decimal.Decimal(250)
  left = decimal.Decimal(72)
  right = decimal.Decimal(20)
  top = decimal.Decimal(20)
  bottom = decimal.Decimal(38)
  values = [
    item[key]
    for item in history
    for key in ('train', 'test')
  ]
  minimum = min(values)
  maximum = max(values)
  value_range = maximum - minimum
  if value_range == 0:
    minimum -= decimal.Decimal('0.5')
    maximum += decimal.Decimal('0.5')
    value_range = decimal.Decimal(1)
  else:
    margin = value_range / decimal.Decimal(20)
    minimum -= margin
    maximum += margin
    value_range = maximum - minimum

  plot_width = width - left - right
  plot_height = height - top - bottom

  def coordinates(index, value):
    x = (
      left + plot_width / 2
      if len(history) == 1 else
      left + plot_width * index / (len(history) - 1))
    y = top + (maximum - value) * plot_height / value_range
    return format(x, '.2f') + ',' + format(y, '.2f')

  train_points = ' '.join(
    coordinates(index, item['train'])
    for index, item in enumerate(history))
  test_points = ' '.join(
    coordinates(index, item['test'])
    for index, item in enumerate(history))
  rows = ''.join(
    '<tr><td>' + str(item['step']) + '</td>'
    '<td class="metric">' + html_text(format(item['train'], '.9f')) + '</td>'
    '<td class="metric">' + html_text(format(item['test'], '.9f')) + '</td></tr>'
    for item in history)

  return (
    '<section class="model-section logloss-section">'
    '<div class="section-title"><div><h2>Logloss history</h2>'
    '<p>Main training metrics after each fitted chunk.</p></div>'
    '<div class="chart-legend"><span class="train-line">Train</span>'
    '<span class="test-line">Test</span></div></div>'
    '<div class="logloss-chart">'
    '<svg viewBox="0 0 900 250" role="img" '
    'aria-label="Train and test Logloss by training step">'
    '<line class="chart-axis" x1="72" y1="20" x2="72" y2="212" />'
    '<line class="chart-axis" x1="72" y1="212" x2="880" y2="212" />'
    '<text x="4" y="25">' + html_text(format(maximum, '.6f')) + '</text>'
    '<text x="4" y="215">' + html_text(format(minimum, '.6f')) + '</text>'
    '<polyline class="chart-line train" points="' + train_points + '" />'
    '<polyline class="chart-line test" points="' + test_points + '" />'
    '</svg></div>'
    '<div class="table-scroll"><table class="logloss-table">'
    '<thead><tr><th>Step</th><th>Train Logloss</th><th>Test Logloss</th></tr></thead>'
    '<tbody>' + rows + '</tbody></table></div></section>')


def render_dataset_sizes(traits):
  dataset_sizes = traits.get('dataset_sizes')
  if not isinstance(dataset_sizes, dict):
    return ''

  rows = []
  for name, label in (
      ('train', 'Train'),
      ('test', 'Test'),
      ('final_test', 'Final test')):
    size = dataset_sizes.get(name)
    if not isinstance(size, dict):
      continue
    try:
      row_count = int(size['rows'])
      click_count = int(size['clicks'])
    except (KeyError, TypeError, ValueError):
      continue
    ctr = (
      decimal.Decimal(click_count) / decimal.Decimal(row_count)
      if row_count else decimal.Decimal(0))
    rows.append(
      '<tr><td>' + label + '</td>'
      '<td class="metric">' + str(row_count) + '</td>'
      '<td class="metric">' + str(click_count) + '</td>'
      '<td class="metric">' + metric_decimal_text(ctr) + '</td></tr>')

  if not rows:
    return ''

  return (
    '<section class="model-section dataset-section">'
    '<h2>Dataset sizes</h2>'
    '<div class="table-scroll"><table class="dataset-table">'
    '<thead><tr><th>Dataset</th><th>Rows</th><th>Clicks</th><th>CTR</th></tr></thead>'
    '<tbody>' + ''.join(rows) + '</tbody></table></div></section>')


def render_ctr_thresholds(traits):
  rows = []
  for item in traits.get('ctr_thresholds', []):
    if not isinstance(item, dict):
      continue
    try:
      ctr_goal = decimal.Decimal(str(item['ctr_goal']))
      impressions = int(item['impressions'])
      clicks = int(item['clicks'])
    except (KeyError, TypeError, ValueError, decimal.InvalidOperation):
      continue
    actual_ctr = item.get('actual_ctr')
    average_predicted_ctr = item.get('average_predicted_ctr')
    rows.append(
      '<tr><td class="metric">' + format(ctr_goal, '.3f') + '</td>'
      '<td class="metric">' + str(impressions) + '</td>'
      '<td class="metric">' + str(clicks) + '</td>'
      '<td class="metric">' + (
        metric_decimal_text(actual_ctr) if actual_ctr is not None else '-') +
      '</td><td class="metric">' + (
        metric_decimal_text(average_predicted_ctr)
        if average_predicted_ctr is not None else '-') +
      '</td></tr>')

  if not rows:
    return ''

  return (
    '<section class="model-section threshold-section">'
    '<div class="section-title"><div><h2>CTR threshold calibration</h2>'
    '<p>Final-test rows where predicted CTR is greater than CTR goal.</p>'
    '</div></div><div class="table-scroll">'
    '<table class="threshold-table"><thead><tr><th>CTR goal</th>'
    '<th>Impressions</th><th>Clicks</th><th>Actual CTR</th>'
    '<th>Average predicted CTR</th></tr></thead><tbody>' +
    ''.join(rows) + '</tbody></table></div></section>')


def render_feature_importance(traits, component_name='model'):
  source_items = traits.get('features_importance', [])
  items = [feature_importance_item(item) for item in source_items]
  max_score = max(
    (item['score'] for item in items if item['score'] > 0),
    default=decimal.Decimal(0))

  rows = []
  for index, item in enumerate(items, 1):
    width = (
      item['score'] * decimal.Decimal(100) / max_score
      if max_score else decimal.Decimal(0))
    search_text = (item['feature'] + ' ' + item['name']).lower()
    rows.append(
      '<tr data-feature="' + html_text(search_text) + '">'
      '<td class="rank">' + str(index) + '</td>'
      '<td class="score"><span>' + html_text(item['score_text']) + '</span>'
      '<i style="width:' + html_text(format(width, '.4f')) + '%"></i></td>'
      '<td class="feature"><code>' + html_text(item['feature']) + '</code></td>'
      '<td class="feature-name">' + html_text(item['name']) + '</td>'
      '<td class="metric">' + html_text(item['yes_share_text']) + '</td>'
      '<td class="metric">' + html_text(item['yes_ctr_text']) + '</td>'
      '<td class="metric">' + html_text(item['no_ctr_text']) + '</td>'
      '</tr>')

  if not rows:
    return '<p class="empty-state">This model has no feature importance data.</p>'

  element_suffix = ''.join(
    character if character.isalnum() else '-'
    for character in component_name)
  filter_id = 'feature-filter-' + element_suffix
  table_id = 'feature-table-' + element_suffix
  count_id = 'feature-count-' + element_suffix
  return (
    '<div class="feature-tools">'
    '<label for="' + filter_id + '">Feature filter</label>'
    '<input id="' + filter_id + '" class="feature-filter" '
    'data-table="' + table_id + '" data-count="' + count_id + '" '
    'type="search" autocomplete="off" '
    'placeholder="Feature, entity, account">'
    '<output id="' + count_id + '">' + str(len(rows)) + ' of ' +
    str(len(rows)) + '</output>'
    '</div>'
    '<div class="table-scroll"><table id="' + table_id + '">'
    '<thead><tr><th>#</th><th>Score</th><th>Feature</th><th>Name</th>'
    '<th>Yes share, %</th><th>Yes CTR</th><th>No CTR</th></tr></thead>'
    '<tbody>' + ''.join(rows) + '</tbody></table></div>')


def render_model_component(component_name, traits, selected=False):
  labels = {
    'common': 'Common',
    'common_denoise': 'Common denoise',
    'common_stable': 'Common stable',
    'campaign_correction': 'Campaign correction',
    'stable_common': 'Stable common',
  }
  descriptions = {
    'common': 'Initial model trained on the full campaign sample.',
    'common_denoise': (
      'Campaign-conditioned residual used to denoise common.'),
    'common_stable': 'Stable common model used by CampaignManager.',
    'campaign_correction': (
      'Residual trained over common; metrics use common + correction.'),
    'stable_common': (
      'Published model trained with out-of-fold campaign correction as baseline.'),
  }
  feature_groups = traits.get('feature_groups', [])
  campaign_name = traits.get('campaign_name')
  label = labels.get(component_name, component_name)
  if traits.get('kind') == 'campaign':
    label = (
      'Campaign ' + str(traits.get('db_campaign_id', component_name)) +
      ((' — ' + str(campaign_name)) if campaign_name else ''))
  description = descriptions.get(component_name, '')
  if traits.get('kind') == 'campaign':
    description = 'Campaign residual trained over common stable.'
  badges = []
  if traits.get('runtime') or traits.get('published'):
    badges.append('<span class="component-published">Runtime</span>')
  training_status = traits.get('status')
  if training_status:
    badges.append(
      '<span class="component-status component-status-' +
      html_text(str(training_status)) + '">' +
      html_text(str(training_status).replace('_', ' ').title()) + '</span>')
  article_state = '' if selected else ' hidden'
  header_and_meta = (
    '<article class="model-component" id="component-' +
    html_text(component_name.replace('_', '-')) + '"' + article_state + '>'
    '<header class="component-header"><div><span class="eyebrow">Model component</span>'
    '<h2>' + html_text(label) + '</h2>'
    '<p>' + html_text(description) + '</p></div>' +
    '<div class="component-badges">' + ''.join(badges) + '</div></header>'
    '<dl class="component-meta">'
    '<div><dt>Status</dt><dd>' +
    html_text(training_status or '-') + '</dd></div>'
    '<div><dt>Train start</dt><dd>' +
    html_text(traits.get('train_start') or '-') + '</dd></div>'
    '<div><dt>Train end</dt><dd>' +
    html_text(traits.get('train_end') or '-') + '</dd></div>'
    '<div><dt>Training impressions</dt><dd>' +
    html_text(traits.get('eligible_training_impressions', '-')) + '</dd></div>'
    '<div><dt>Artifact</dt><dd><code>' +
    html_text(traits.get('file', '-')) + '</code></dd></div>'
    '<div><dt>Metrics prediction</dt><dd><code>' +
    html_text(traits.get('metrics_prediction', '-')) + '</code></dd></div>'
    '<div><dt>Training baseline</dt><dd><code>' +
    html_text(traits.get(
      'training_baseline',
      traits.get('baseline_model', '-'))) + '</code></dd></div>'
    '<div><dt>Runtime weight</dt><dd><code>' +
    html_text(traits.get('weight', '-')) + '</code></dd></div>'
    '<div><dt>Feature groups</dt><dd>' + str(len(feature_groups)) + '</dd></div>'
    '<div><dt>Ranked features</dt><dd>' +
    str(len(traits.get('features_importance', []))) + '</dd></div></dl>')
  if not traits.get('file'):
    return header_and_meta + '</article>'
  return (
    header_and_meta +
    '<section class="component-section"><h3>Feature groups</h3><p>' +
    render_feature_groups(feature_groups) + '</p></section>' +
    render_dataset_sizes(traits) +
    render_ctr_thresholds(traits) +
    render_logloss_history(traits) +
    '<section class="model-section feature-section"><div class="section-title">'
    '<div><h3>Feature importance</h3>'
    '<p>Relative contribution reported by this component.</p></div></div>' +
    render_feature_importance(traits, component_name) + '</section></article>')


def render_model_collection(components):
  component_items = [
    (name, traits)
    for name, traits in components.items()
    if isinstance(traits, dict)
  ]
  if not component_items:
    return ''

  selected_name = None
  for status in ('training', 'interrupted'):
    selected_name = next((
      name
      for name, traits in component_items
      if traits.get('status') == status
    ), None)
    if selected_name is not None:
      break
  if selected_name is None:
    for preferred_name in ('common_stable', 'stable_common'):
      if preferred_name in components:
        selected_name = preferred_name
        break
  if selected_name is None:
    selected_name = next((
      name
      for name, traits in component_items
      if traits.get('runtime')
    ), component_items[0][0])

  def render_component_link(name, traits):
    selected = name == selected_name
    current = ' aria-current="page"' if selected else ''
    return (
      '<a class="component-link" data-model="' + html_text((
        name + ' ' + str(traits.get('db_campaign_id', '')) +
        ' ' + str(traits.get('campaign_name') or '')).lower()) +
      '" data-status="' + html_text(traits.get('status', '')) +
      '" data-runtime="' + ('true' if traits.get('runtime') else 'false') +
      '" href="#component-' + html_text(name.replace('_', '-')) + '"' +
      current + '>' +
      '<span>' + html_text(
        name.replace('_', ' ') + (
          ' — ' + str(traits.get('campaign_name'))
          if traits.get('campaign_name') else '')) + '</span>' +
      '<small>' + html_text(traits.get('status') or '') + '</small></a>')

  common_items = [
    item
    for item in component_items
    if item[1].get('kind') != 'campaign'
  ]
  campaign_items = [
    item
    for item in component_items
    if item[1].get('kind') == 'campaign'
  ]

  def render_group(title, items, group_name):
    if not items:
      return ''
    return (
      '<section class="component-group" data-component-group="' +
      group_name + '"><h3>' + title + '<span>' + str(len(items)) +
      '</span></h3><nav class="component-nav" aria-label="' + title + '">' +
      ''.join(render_component_link(name, traits) for name, traits in items) +
      '</nav></section>')

  return (
    '<div class="component-workspace">'
    '<aside class="component-sidebar" aria-label="Models within bundle">'
    '<div class="component-tools"><label for="component-filter">Models within bundle</label>'
    '<input id="component-filter" type="search" autocomplete="off" '
    'placeholder="Campaign ID or name">'
    '<select id="component-status-filter" aria-label="Model status">'
    '<option value="all">All statuses</option>'
    '<option value="planned">Planned</option>'
    '<option value="training">Training</option>'
    '<option value="completed">Completed</option>'
    '<option value="interrupted">Interrupted</option>'
    '<option value="runtime">Runtime</option></select>'
    '<output id="component-count">' + str(len(component_items)) + ' of ' +
    str(len(component_items)) + '</output></div>' +
    render_group('Core models', common_items, 'core') +
    render_group('Campaign models', campaign_items, 'campaign') +
    '</aside><section class="component-detail">' +
    ''.join(
      render_model_component(name, traits, name == selected_name)
      for name, traits in component_items) +
    '</section></div>')


def render_model_details(properties):
  summary = properties['summary']
  traits = properties.get('traits', {})
  if summary.get('status') in ('in_progress', 'interrupted'):
    interrupted = summary['status'] == 'interrupted'
    trait_models = traits.get('models')
    components = {
      item['name']: item
      for item in trait_models
      if isinstance(item, dict) and isinstance(item.get('name'), str)
    } if isinstance(trait_models, list) else {}
    train_end = ''
    if summary.get('train_end'):
      train_end = (
        '<div><dt>Train end</dt><dd>' +
        html_text(summary['train_end']) + '</dd></div>')
    details = (
      '<header class="model-header">'
      '<div><span class="eyebrow">' +
      ('Training interrupted' if interrupted else 'Training in progress') +
      '</span>'
      '<h1>' + html_text(summary['id']) + '</h1></div></header>'
      '<dl class="model-meta training-meta">'
      '<div><dt>Train start</dt><dd>' +
      html_text(summary.get('train_start') or '-') + '</dd></div>' +
      train_end +
      '<div><dt>Planned models</dt><dd>' +
      str(summary.get('models_count', 0)) + '</dd></div>'
      '<div><dt>Campaign models</dt><dd>' +
      str(summary.get('campaign_models_count', 0)) + '</dd></div>'
      '<div><dt>Completed models</dt><dd>' +
      str(summary.get('completed_models_count', 0)) + '</dd></div>'
      '<div><dt>Interrupted models</dt><dd>' +
      str(summary.get('interrupted_models_count', 0)) + '</dd></div></dl>')
    if components:
      details += render_model_collection(components)
    return details

  model_id = summary['id']
  feature_groups = summary.get('feature_groups', [])
  components = traits.get('components')
  if not isinstance(components, dict):
    components = {}
  trait_models = traits.get('models')
  if isinstance(trait_models, list):
    components = {
      item['name']: item
      for item in trait_models
      if isinstance(item, dict) and isinstance(item.get('name'), str)
    }
  extra_traits = {
    key: value
    for key, value in traits.items()
    if key not in (
      'features_importance',
      'logloss_history',
      'dataset_sizes',
      'ctr_thresholds',
      'status',
      'train_start',
      'train_end',
      'components',
      'models')
  }
  extra_traits_html = ''
  if extra_traits:
    extra_traits_html = (
      '<section class="model-section"><h2>Additional traits</h2>'
      '<pre>' + html_text(json_dumps(extra_traits)) + '</pre></section>')

  details_prefix = (
    '<header class="model-header">'
    '<div><span class="eyebrow">Selected model</span>'
    '<h1>' + html_text(model_id) + '</h1></div>'
    '<nav class="model-actions" aria-label="Model resources">'
    '<a href="/models/' + urllib.parse.quote(model_id, safe='') + '/config">Config</a>'
    '<a href="/models/' + urllib.parse.quote(model_id, safe='') + '/traits">Traits</a>'
    '</nav></header>'
    '<dl class="model-meta">'
    '<div><dt>Algorithm</dt><dd>' + html_text(summary.get('algorithm_id') or '-') + '</dd></div>'
    '<div><dt>Method</dt><dd>' + html_text(summary.get('method') or '-') + '</dd></div>'
    '<div><dt>Feature groups</dt><dd>' + str(len(feature_groups)) + '</dd></div>'
    '<div><dt>Ranked features</dt><dd>' +
    str(summary.get('features_importance_count', 0)) + '</dd></div>'
    '<div><dt>Components</dt><dd>' +
    str(summary.get('components_count', 0) or 1) + '</dd></div>'
    '<div><dt>Train start</dt><dd>' +
    html_text(summary.get('train_start') or '-') + '</dd></div>'
    '<div><dt>Train end</dt><dd>' +
    html_text(summary.get('train_end') or '-') + '</dd></div>'
    '</dl>'
    '<section class="model-section"><h2>Runtime feature groups</h2><p>' +
    render_feature_groups(feature_groups) + '</p></section>')

  if components:
    return (
      details_prefix +
      render_model_collection(components) +
      extra_traits_html)

  return (
    details_prefix
    + render_dataset_sizes(traits)
    + render_ctr_thresholds(traits)
    + render_logloss_history(traits) +
    '<section class="model-section feature-section"><div class="section-title">'
    '<div><h2>Feature importance</h2>'
    '<p>Relative contribution reported by the trained model.</p></div></div>'
    + render_feature_importance(traits) + '</section>' + extra_traits_html)


def render_index_page(models, selected_properties=None):
  selected_model_id = (
    selected_properties['summary']['id']
    if selected_properties else None)
  content = (
    render_model_details(selected_properties)
    if selected_properties else
    '<div class="welcome"><h1>CTR models</h1>'
    '<p>Select a model to inspect its training status or published traits.</p>'
    '</div>')
  return '<!doctype html>' + r'''
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>CTR Models</title>
  <style>
    :root {
      color-scheme: light;
      --ink: #17212b;
      --muted: #65717c;
      --line: #d8dee4;
      --panel: #f4f6f7;
      --selected: #dcefe5;
      --accent: #087a55;
      --score: #d97706;
      font-family: Inter, ui-sans-serif, system-ui, sans-serif;
    }
    * { box-sizing: border-box; }
    body { margin: 0; color: var(--ink); background: #fff; letter-spacing: 0; }
    a { color: var(--accent); }
    .shell { min-height: 100vh; display: grid; grid-template-columns: 300px minmax(0, 1fr); }
    .sidebar { background: var(--panel); border-right: 1px solid var(--line); }
    .brand { padding: 24px; border-bottom: 1px solid var(--line); }
    .brand strong { display: block; font-size: 18px; }
    .brand span { color: var(--muted); font-size: 13px; }
    .sidebar h2 { margin: 22px 24px 10px; font-size: 12px; text-transform: uppercase; }
    .model-list { margin: 0; padding: 0 12px 24px; list-style: none; }
    .model-link { display: grid; padding: 12px; border-left: 3px solid transparent;
      color: inherit; text-decoration: none; }
    .model-link:hover { background: #e9edef; }
    .model-link.selected { background: var(--selected); border-left-color: var(--accent); }
    .model-link strong { font-size: 14px; }
    .model-link span { color: var(--muted); font-size: 12px; }
    .model-link .in-progress { color: var(--score); font-weight: 700; }
    .model-link .interrupted { color: #b42318; font-weight: 700; }
    .empty-list { padding: 0 24px; color: var(--muted); }
    main { min-width: 0; padding: 32px 40px 64px; }
    .model-header { display: flex; align-items: end; justify-content: space-between;
      gap: 24px; padding-bottom: 24px; border-bottom: 1px solid var(--line); }
    .eyebrow { color: var(--accent); font-size: 12px; font-weight: 700;
      text-transform: uppercase; }
    h1 { margin: 4px 0 0; font-size: 30px; }
    h2 { margin: 0; font-size: 18px; }
    .model-actions { display: flex; gap: 18px; }
    .model-meta { display: grid;
      grid-template-columns: repeat(auto-fit, minmax(160px, 1fr));
      margin: 0; border-bottom: 1px solid var(--line); }
    .model-meta div { padding: 20px 24px 20px 0; }
    .model-meta dt { color: var(--muted); font-size: 12px; }
    .model-meta dd { margin: 4px 0 0; font-size: 18px; font-weight: 650; }
    .model-section { padding: 28px 0; border-bottom: 1px solid var(--line); }
    .model-section p { color: var(--muted); }
    .component-workspace { display: grid; grid-template-columns: 280px minmax(0, 1fr);
      gap: 28px; align-items: start; margin-top: 28px; }
    .component-sidebar { position: sticky; top: 20px; max-height: calc(100vh - 40px);
      overflow: auto; border: 1px solid var(--line); background: var(--panel); }
    .component-tools { display: grid; grid-template-columns: 1fr; gap: 9px;
      padding: 16px; border-bottom: 1px solid var(--line); }
    .component-tools label { font-size: 13px; font-weight: 700; }
    .component-tools input, .component-tools select { width: 100%; height: 36px;
      border: 1px solid #aeb8c1; background: #fff; padding: 0 10px; font: inherit; }
    .component-tools output { color: var(--muted); font-size: 12px; }
    .component-group { padding: 14px 10px; border-bottom: 1px solid var(--line); }
    .component-group:last-child { border-bottom: 0; }
    .component-group h3 { display: flex; justify-content: space-between; margin: 0 7px 8px;
      color: var(--muted); font-size: 11px; text-transform: uppercase; }
    .component-group h3 span { font-variant-numeric: tabular-nums; }
    .component-nav { display: grid; gap: 3px; }
    .component-nav a { display: grid; grid-template-columns: minmax(0, 1fr) auto;
      gap: 8px; padding: 9px 8px; border-left: 3px solid transparent;
      color: inherit; text-decoration: none; text-transform: capitalize; }
    .component-nav a:hover { background: #e9edef; }
    .component-nav a[aria-current="page"] { border-left-color: var(--accent);
      background: var(--selected); }
    .component-nav a span { overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }
    .component-nav a small { color: var(--muted); font-size: 10px; }
    .component-detail { min-width: 0; }
    .model-component { margin: 0; padding: 28px; border: 1px solid var(--line);
      scroll-margin-top: 20px; }
    [hidden] { display: none !important; }
    .component-header { display: flex; justify-content: space-between; gap: 20px;
      padding-bottom: 18px; border-bottom: 1px solid var(--line); }
    .component-header h2 { margin-top: 4px; font-size: 23px; }
    .component-header p { margin: 7px 0 0; color: var(--muted); }
    .component-badges { display: flex; align-self: start; flex-wrap: wrap;
      justify-content: end; gap: 7px; }
    .component-published { padding: 5px 9px; color: var(--accent);
      background: var(--selected); font-size: 12px; font-weight: 700; }
    .component-status { padding: 5px 9px; color: var(--muted); background: var(--panel);
      font-size: 12px; font-weight: 700; }
    .component-status-training { color: #925500; background: #fff1d6; }
    .component-status-completed { color: var(--accent); background: var(--selected); }
    .component-status-interrupted { color: #b42318; background: #fee4e2; }
    .component-meta { display: grid; grid-template-columns: repeat(auto-fit, minmax(180px, 1fr));
      margin: 0; border-bottom: 1px solid var(--line); }
    .component-meta div { padding: 16px 16px 16px 0; }
    .component-meta dt { color: var(--muted); font-size: 12px; }
    .component-meta dd { margin: 4px 0 0; }
    .component-section { padding: 22px 0; border-bottom: 1px solid var(--line); }
    .component-section h3, .model-component h3 { margin: 0; font-size: 16px; }
    .feature-groups { color: var(--ink); font-family: ui-monospace, monospace; }
    .section-title p { margin: 5px 0 0; }
    .section-title { display: flex; align-items: start; justify-content: space-between;
      gap: 24px; }
    .chart-legend { display: flex; gap: 18px; font-size: 12px; }
    .chart-legend span::before { content: ""; display: inline-block; width: 18px;
      height: 3px; margin: 0 7px 3px 0; background: var(--accent); }
    .chart-legend .test-line::before { background: var(--score); }
    .logloss-chart { margin: 18px 0 10px; }
    .logloss-chart svg { display: block; width: 100%; height: auto; max-height: 300px; }
    .logloss-chart text { fill: var(--muted); font-size: 11px; }
    .chart-axis { stroke: var(--line); stroke-width: 1; }
    .chart-line { fill: none; stroke-width: 2.5; vector-effect: non-scaling-stroke; }
    .chart-line.train { stroke: var(--accent); }
    .chart-line.test { stroke: var(--score); }
    .logloss-table { max-width: 620px; }
    .dataset-table { margin-top: 18px; max-width: 620px; }
    .threshold-table { margin-top: 18px; max-width: 900px; }
    .feature-tools { display: grid; grid-template-columns: auto minmax(220px, 440px) 1fr;
      gap: 12px; align-items: center; margin: 20px 0 12px; }
    .feature-tools label { font-size: 13px; font-weight: 650; }
    .feature-tools input { width: 100%; height: 36px; border: 1px solid #aeb8c1;
      padding: 0 10px; font: inherit; }
    .feature-tools input:focus { outline: 2px solid var(--accent); outline-offset: 1px; }
    .feature-tools output { color: var(--muted); font-size: 12px; }
    .table-scroll { overflow-x: auto; }
    table { width: 100%; border-collapse: collapse; font-size: 13px; }
    th { position: sticky; top: 0; background: #fff; color: var(--muted);
      text-align: left; font-size: 11px; text-transform: uppercase; }
    th, td { padding: 9px 12px 9px 0; border-bottom: 1px solid #e8ecef; }
    .rank { width: 48px; color: var(--muted); }
    .score { position: relative; width: 220px; font-variant-numeric: tabular-nums; }
    .score span { position: relative; z-index: 1; }
    .score i { position: absolute; left: 0; bottom: 4px; height: 3px;
      background: var(--score); }
    .feature code { color: #075f7a; white-space: nowrap; }
    .feature-name { min-width: 260px; }
    .metric { min-width: 120px; font-variant-numeric: tabular-nums; }
    .empty-state, .welcome { color: var(--muted); }
    .welcome { max-width: 620px; padding-top: 15vh; }
    .welcome h1 { color: var(--ink); }
    pre { overflow: auto; padding: 16px; background: var(--panel); font-size: 12px; }
    @media (max-width: 1180px) {
      .component-workspace { grid-template-columns: 1fr; }
      .component-sidebar { position: static; max-height: none; }
      .component-group { max-height: 250px; overflow: auto; }
    }
    @media (max-width: 820px) {
      .shell { grid-template-columns: 1fr; }
      .sidebar { border-right: 0; border-bottom: 1px solid var(--line); }
      .model-list { display: flex; gap: 8px; overflow-x: auto; }
      .model-list li { min-width: 210px; }
      main { padding: 24px 18px 48px; }
      .model-header { align-items: start; flex-direction: column; }
      .model-meta { grid-template-columns: repeat(2, 1fr); }
      .feature-tools { grid-template-columns: 1fr; }
    }
  </style>
</head>
<body>
  <div class="shell">
    <aside class="sidebar">
      <div class="brand"><strong>CTR Models</strong><span>Predict model registry</span></div>
      <h2>Models</h2>
      ''' + render_model_list(models, selected_model_id) + r'''
    </aside>
    <main>''' + content + r'''</main>
  </div>
  <script>
    for (const filter of document.querySelectorAll('.feature-filter')) {
      const table = document.getElementById(filter.dataset.table);
      const rows = Array.from(table.querySelectorAll('tbody tr'));
      const count = document.getElementById(filter.dataset.count);
      filter.addEventListener('input', () => {
        const query = filter.value.trim().toLowerCase();
        let visible = 0;
        for (const row of rows) {
          const show = !query || row.dataset.feature.includes(query);
          row.hidden = !show;
          visible += show ? 1 : 0;
        }
        count.value = `${visible} of ${rows.length}`;
        count.textContent = count.value;
      });
    }
    const componentFilter = document.getElementById('component-filter');
    if (componentFilter) {
      const links = Array.from(document.querySelectorAll('.component-link'));
      const articles = Array.from(document.querySelectorAll('.model-component'));
      const statusFilter = document.getElementById('component-status-filter');
      const count = document.getElementById('component-count');
      const groups = Array.from(document.querySelectorAll('.component-group'));

      const selectComponent = (link, updateHash = true) => {
        for (const item of links) {
          if (item === link) {
            item.setAttribute('aria-current', 'page');
          } else {
            item.removeAttribute('aria-current');
          }
        }
        for (const article of articles) {
          article.hidden = '#' + article.id !== link.hash;
        }
        if (updateHash) {
          history.replaceState(null, '', link.hash);
        }
      };

      const applyComponentFilter = () => {
        const query = componentFilter.value.trim().toLowerCase();
        const status = statusFilter.value;
        let visible = 0;
        for (const link of links) {
          const matchesQuery = !query || link.dataset.model.includes(query);
          const matchesStatus = (
            status === 'all' ||
            link.dataset.status === status ||
            (status === 'runtime' && link.dataset.runtime === 'true'));
          const show = matchesQuery && matchesStatus;
          link.hidden = !show;
          visible += show ? 1 : 0;
        }
        for (const group of groups) {
          group.hidden = !Array.from(
            group.querySelectorAll('.component-link')).some(link => !link.hidden);
        }
        count.value = `${visible} of ${links.length}`;
        count.textContent = count.value;

        const selected = links.find(
          link => link.getAttribute('aria-current') === 'page');
        if (!selected || selected.hidden) {
          const firstVisible = links.find(link => !link.hidden);
          if (firstVisible) {
            selectComponent(firstVisible, false);
          } else {
            for (const article of articles) {
              article.hidden = true;
            }
          }
        }
      };

      for (const link of links) {
        link.addEventListener('click', event => {
          event.preventDefault();
          selectComponent(link);
        });
      }
      componentFilter.addEventListener('input', applyComponentFilter);
      statusFilter.addEventListener('change', applyComponentFilter);

      const hashLink = links.find(link => link.hash === window.location.hash);
      if (hashLink) {
        selectComponent(hashLink, false);
      }
    }
  </script>
</body>
</html>
'''


def create_application(repository):
  application = FastAPI(
    title='CTR Predict Model Generator',
    default_response_class=DecimalJSONResponse)

  def call_repository(fun, *args):
    try:
      return fun(*args)
    except ModelNotFound as error:
      raise HTTPException(status_code=404, detail=str(error))
    except RuntimeError as error:
      raise HTTPException(status_code=500, detail=str(error))

  @application.get('/health')
  async def health():
    return DecimalJSONResponse({'status': 'ok'})

  @application.get('/', include_in_schema=False)
  async def root(model_id: str | None = Query(default=None, alias='model')):
    model_ids = repository.all_model_ids()
    summaries = [
      call_repository(repository.model_summary, item)
      for item in model_ids
    ]
    selected_properties = (
      call_repository(repository.model_properties, model_id)
      if model_id is not None else None)
    return HTMLResponse(render_index_page(summaries, selected_properties))

  @application.get('/models')
  async def models():
    return DecimalJSONResponse({
      'items': [
        call_repository(repository.model_summary, model_id)
        for model_id in repository.all_model_ids()
      ],
    })

  @application.get('/models/latest')
  async def latest_model():
    model_id = call_repository(repository.latest_model_id)
    return DecimalJSONResponse(
      call_repository(repository.model_properties, model_id))

  @application.get('/models/{model_id}')
  async def model(model_id: str):
    return DecimalJSONResponse(
      call_repository(repository.model_properties, model_id))

  @application.get('/models/{model_id}/features')
  async def model_features(
      model_id: str,
      offset: int = Query(default=0, ge=0),
      limit: int = Query(default=100, ge=1, le=1000),
      component: str | None = Query(default=None),
  ):
    return DecimalJSONResponse(
      call_repository(
        repository.features,
        model_id,
        offset,
        limit,
        component))

  @application.get('/models/{model_id}/config')
  async def model_config(model_id: str):
    path = call_repository(repository.model_file, model_id, 'config.json')
    return FileResponse(path, media_type='application/json')

  @application.get('/models/{model_id}/traits')
  async def model_traits(model_id: str):
    path = call_repository(repository.model_file, model_id, 'traits.json')
    return FileResponse(path, media_type='application/json')

  return application
