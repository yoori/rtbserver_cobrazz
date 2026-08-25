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
    if model.get('status') == 'in_progress':
      details = (
        '<span class="model-status in-progress">In progress</span>'
        '<span>Started ' + html_text(model.get('train_start') or '-') + '</span>')
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


def render_model_component(component_name, traits):
  labels = {
    'common': 'Common',
    'campaign_correction': 'Campaign correction',
    'stable_common': 'Stable common',
  }
  descriptions = {
    'common': 'Initial model trained on the full campaign sample.',
    'campaign_correction': (
      'Residual trained over common; metrics use common + correction.'),
    'stable_common': (
      'Published model trained with out-of-fold campaign correction as baseline.'),
  }
  feature_groups = traits.get('feature_groups', [])
  published = (
    '<span class="component-published">Published</span>'
    if traits.get('published') else '')
  return (
    '<article class="model-component" id="component-' +
    html_text(component_name.replace('_', '-')) + '">'
    '<header class="component-header"><div><span class="eyebrow">Model component</span>'
    '<h2>' + html_text(labels.get(component_name, component_name)) + '</h2>'
    '<p>' + html_text(descriptions.get(component_name, '')) + '</p></div>' +
    published + '</header>'
    '<dl class="component-meta">'
    '<div><dt>Artifact</dt><dd><code>' +
    html_text(traits.get('file', '-')) + '</code></dd></div>'
    '<div><dt>Metrics prediction</dt><dd><code>' +
    html_text(traits.get('metrics_prediction', '-')) + '</code></dd></div>'
    '<div><dt>Training baseline</dt><dd><code>' +
    html_text(traits.get('training_baseline', '-')) + '</code></dd></div>'
    '<div><dt>Feature groups</dt><dd>' + str(len(feature_groups)) + '</dd></div>'
    '<div><dt>Ranked features</dt><dd>' +
    str(len(traits.get('features_importance', []))) + '</dd></div></dl>'
    '<section class="component-section"><h3>Feature groups</h3><p>' +
    render_feature_groups(feature_groups) + '</p></section>' +
    render_dataset_sizes(traits) +
    render_ctr_thresholds(traits) +
    render_logloss_history(traits) +
    '<section class="model-section feature-section"><div class="section-title">'
    '<div><h3>Feature importance</h3>'
    '<p>Relative contribution reported by this component.</p></div></div>' +
    render_feature_importance(traits, component_name) + '</section></article>')


def render_model_details(properties):
  summary = properties['summary']
  if summary.get('status') == 'in_progress':
    return (
      '<header class="model-header">'
      '<div><span class="eyebrow">Training in progress</span>'
      '<h1>' + html_text(summary['id']) + '</h1></div></header>'
      '<dl class="model-meta training-meta">'
      '<div><dt>Train start</dt><dd>' +
      html_text(summary.get('train_start') or '-') + '</dd></div></dl>')

  traits = properties['traits']
  model_id = summary['id']
  feature_groups = summary.get('feature_groups', [])
  components = traits.get('components')
  if not isinstance(components, dict):
    components = {}
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
      'components')
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
      '<nav class="component-nav" aria-label="Model components">' +
      ''.join(
        '<a href="#component-' + html_text(name.replace('_', '-')) + '">' +
        html_text(name.replace('_', ' ')) + '</a>'
        for name in components) + '</nav>' +
      ''.join(
        render_model_component(name, component_traits)
        for name, component_traits in components.items()
        if isinstance(component_traits, dict)) +
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
    .component-nav { display: flex; flex-wrap: wrap; gap: 10px; padding: 20px 0;
      border-bottom: 1px solid var(--line); }
    .component-nav a { padding: 7px 11px; border: 1px solid var(--line);
      text-decoration: none; text-transform: capitalize; }
    .model-component { margin-top: 34px; padding: 28px; border: 1px solid var(--line);
      scroll-margin-top: 20px; }
    .component-header { display: flex; justify-content: space-between; gap: 20px;
      padding-bottom: 18px; border-bottom: 1px solid var(--line); }
    .component-header h2 { margin-top: 4px; font-size: 23px; }
    .component-header p { margin: 7px 0 0; color: var(--muted); }
    .component-published { align-self: start; padding: 5px 9px; color: var(--accent);
      background: var(--selected); font-size: 12px; font-weight: 700; }
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
