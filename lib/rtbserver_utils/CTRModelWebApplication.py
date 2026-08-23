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


def feature_importance_item(item):
  if not isinstance(item, dict):
    return {
      'score': decimal.Decimal(0),
      'score_text': '0',
      'feature': str(item),
      'name': '',
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
  }


def render_model_list(models, selected_model_id):
  if not models:
    return '<p class="empty-list">No published models</p>'

  items = []
  for model in models:
    model_id = model['id']
    selected = model_id == selected_model_id
    class_name = 'model-link selected' if selected else 'model-link'
    current = ' aria-current="page"' if selected else ''
    url = '/?model=' + urllib.parse.quote(model_id, safe='')
    items.append(
      '<li><a class="' + class_name + '" href="' + url + '"' + current + '>'
      '<strong>' + html_text(model_id) + '</strong>'
      '<span>' + html_text(model.get('algorithm_id') or 'unknown') + '</span>'
      '<span>' + str(model.get('features_importance_count', 0)) + ' features</span>'
      '</a></li>')
  return '<ol class="model-list">' + ''.join(items) + '</ol>'


def render_feature_groups(feature_groups):
  if not feature_groups:
    return '<span class="muted">None</span>'
  return '<span class="feature-groups">' + ', '.join(
    html_text(' + '.join(str(value) for value in group))
    for group in feature_groups
  ) + '</span>'


def render_feature_importance(traits):
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
      '</tr>')

  if not rows:
    return '<p class="empty-state">This model has no feature importance data.</p>'

  return (
    '<div class="feature-tools">'
    '<label for="feature-filter">Feature filter</label>'
    '<input id="feature-filter" type="search" autocomplete="off" '
    'placeholder="Feature, entity, account">'
    '<output id="feature-count">' + str(len(rows)) + ' of ' + str(len(rows)) + '</output>'
    '</div>'
    '<div class="table-scroll"><table id="feature-table">'
    '<thead><tr><th>#</th><th>Score</th><th>Feature</th><th>Name</th></tr></thead>'
    '<tbody>' + ''.join(rows) + '</tbody></table></div>')


def render_model_details(properties):
  summary = properties['summary']
  traits = properties['traits']
  model_id = summary['id']
  feature_groups = summary.get('feature_groups', [])
  extra_traits = {
    key: value
    for key, value in traits.items()
    if key != 'features_importance'
  }
  extra_traits_html = ''
  if extra_traits:
    extra_traits_html = (
      '<section class="model-section"><h2>Additional traits</h2>'
      '<pre>' + html_text(json_dumps(extra_traits)) + '</pre></section>')

  return (
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
    '</dl>'
    '<section class="model-section"><h2>Runtime feature groups</h2><p>' +
    render_feature_groups(feature_groups) + '</p></section>'
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
    '<p>Select a published model to inspect its runtime configuration and traits.</p>'
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
    .empty-list { padding: 0 24px; color: var(--muted); }
    main { min-width: 0; padding: 32px 40px 64px; }
    .model-header { display: flex; align-items: end; justify-content: space-between;
      gap: 24px; padding-bottom: 24px; border-bottom: 1px solid var(--line); }
    .eyebrow { color: var(--accent); font-size: 12px; font-weight: 700;
      text-transform: uppercase; }
    h1 { margin: 4px 0 0; font-size: 30px; }
    h2 { margin: 0; font-size: 18px; }
    .model-actions { display: flex; gap: 18px; }
    .model-meta { display: grid; grid-template-columns: repeat(4, minmax(120px, 1fr));
      margin: 0; border-bottom: 1px solid var(--line); }
    .model-meta div { padding: 20px 24px 20px 0; }
    .model-meta dt { color: var(--muted); font-size: 12px; }
    .model-meta dd { margin: 4px 0 0; font-size: 18px; font-weight: 650; }
    .model-section { padding: 28px 0; border-bottom: 1px solid var(--line); }
    .model-section p { color: var(--muted); }
    .feature-groups { color: var(--ink); font-family: ui-monospace, monospace; }
    .section-title p { margin: 5px 0 0; }
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
      <h2>Published models</h2>
      ''' + render_model_list(models, selected_model_id) + r'''
    </aside>
    <main>''' + content + r'''</main>
  </div>
  <script>
    const filter = document.getElementById('feature-filter');
    if (filter) {
      const rows = Array.from(document.querySelectorAll('#feature-table tbody tr'));
      const count = document.getElementById('feature-count');
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
    model_ids = repository.model_ids()
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
        for model_id in repository.model_ids()
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
  ):
    return DecimalJSONResponse(
      call_repository(repository.features, model_id, offset, limit))

  @application.get('/models/{model_id}/config')
  async def model_config(model_id: str):
    path = call_repository(repository.model_file, model_id, 'config.json')
    return FileResponse(path, media_type='application/json')

  @application.get('/models/{model_id}/traits')
  async def model_traits(model_id: str):
    path = call_repository(repository.model_file, model_id, 'traits.json')
    return FileResponse(path, media_type='application/json')

  return application
