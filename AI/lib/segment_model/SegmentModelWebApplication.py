import html
import json
import math
import urllib.parse

from fastapi import FastAPI, HTTPException, Query
from fastapi.responses import FileResponse, HTMLResponse, Response

from .SegmentModelRepository import SegmentModelNotFound


class SegmentModelJSONResponse(Response):
  media_type = 'application/json'

  def render(self, content):
    return json.dumps(
      content,
      allow_nan=False,
      separators=(',', ':'),
      sort_keys=True).encode('utf-8')


def create_application(repository, url_path='/'):
  url_path = normalize_url_path(url_path)
  application = FastAPI(
    title='Segment Model Viewer',
    default_response_class=SegmentModelJSONResponse)

  def get(path, **kwargs):
    def decorator(fun):
      application.get(path, **kwargs)(fun)
      if url_path != '/':
        application.get(url_path.rstrip('/') + path, **kwargs)(fun)
      return fun
    return decorator

  def call_repository(fun, *args):
    try:
      return fun(*args)
    except SegmentModelNotFound as error:
      raise HTTPException(status_code=404, detail=str(error)) from error
    except RuntimeError as error:
      raise HTTPException(status_code=500, detail=str(error)) from error

  @get('/health')
  async def health():
    return SegmentModelJSONResponse({'status': 'ok'})

  @get('/', include_in_schema=False)
  async def root(
      model_id: str | None = Query(default=None, alias='model'),
      search: str | None = Query(default=None)):
    summaries = [
      call_repository(repository.model_summary, item)
      for item in repository.all_model_ids()
    ]
    properties = None
    segments = None
    if model_id is not None:
      properties = call_repository(repository.model_properties, model_id)
      segments = call_repository(
        repository.segments,
        model_id,
        0,
        200,
        search)
    return HTMLResponse(
      render_index_page(summaries, properties, segments, search, url_path))

  @get('/models')
  async def models():
    return SegmentModelJSONResponse({
      'items': [
        call_repository(repository.model_summary, model_id)
        for model_id in repository.all_model_ids()
      ],
    })

  @get('/models/latest')
  async def latest_model():
    model_id = call_repository(repository.latest_model_id)
    return SegmentModelJSONResponse(
      call_repository(repository.model_properties, model_id))

  @get('/models/{model_id}')
  async def model(model_id: str):
    return SegmentModelJSONResponse(
      call_repository(repository.model_properties, model_id))

  @get('/models/{model_id}/segments')
  async def model_segments(
      model_id: str,
      offset: int = Query(default=0, ge=0),
      limit: int = Query(default=100, ge=1, le=1000),
      search: str | None = Query(default=None)):
    return SegmentModelJSONResponse(
      call_repository(
        repository.segments,
        model_id,
        offset,
        limit,
        search))

  @get('/models/{model_id}/segments/{segment_id}')
  async def model_segment(model_id: str, segment_id: int):
    return SegmentModelJSONResponse(
      call_repository(repository.segment, model_id, segment_id))

  @get(
    '/models/{model_id}/segments/{segment_id}/view',
    include_in_schema=False)
  async def model_segment_page(model_id: str, segment_id: int):
    segment = call_repository(repository.segment, model_id, segment_id)
    return HTMLResponse(render_segment_page(model_id, segment, url_path))

  @get('/models/{model_id}/files/{file_name}')
  async def model_file(model_id: str, file_name: str):
    path = call_repository(repository.model_file, model_id, file_name)
    return FileResponse(path, media_type='application/json')

  return application


def render_index_page(
    models,
    selected_properties=None,
    segments=None,
    search=None,
    url_path='/'):
  selected_id = (
    selected_properties.get('summary', {}).get('id')
    if selected_properties else None)
  model_list = ''.join(
    render_model_list_item(model, model.get('id') == selected_id, url_path)
    for model in models)
  if not model_list:
    model_list = '<p class="empty">No models</p>'
  details = (
    render_model_details(selected_properties, segments, search, url_path)
    if selected_properties else
    '<main class="empty-state"><h1>Segment models</h1><p>No model selected.</p></main>')
  refresh = ''
  if selected_properties and selected_properties['summary'].get('status') == 'in_progress':
    refresh = '<meta http-equiv="refresh" content="10">'
  return '<!doctype html>' + f'''
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Segment Model Viewer</title>
  {refresh}
  <style>{STYLES}</style>
</head>
<body>
  <div class="layout">
    <aside class="sidebar">
      <header><strong>Segment models</strong><span>{len(models)}</span></header>
      <nav>{model_list}</nav>
    </aside>
    {details}
  </div>
</body>
</html>'''


def render_model_list_item(model, selected, url_path):
  model_id = str(model.get('id', ''))
  url = application_url(url_path, '?model=' + urllib.parse.quote(model_id))
  status = model.get('status', 'unknown')
  values = []
  if model.get('segments_count'):
    values.append(str(model['segments_count']) + ' segments')
  if model.get('soft_logloss') is not None:
    values.append('soft ' + number(model['soft_logloss']))
  if model.get('stage'):
    values.append(str(model['stage']))
  return (
    f'<a class="model-link status-{attribute(status)}'
    f'{" selected" if selected else ""}" href="{attribute(url)}">'
    f'<span class="model-id">{text(model_id)}</span>'
    f'<span class="model-status">{text(status.replace("_", " "))}</span>'
    f'<small>{text(" | ".join(values))}</small></a>')


def render_model_details(properties, segments, search, url_path):
  summary = properties.get('summary', {})
  traits = properties.get('traits', {})
  model_id = str(summary.get('id', ''))
  status = summary.get('status', 'unknown')
  progress = traits.get('progress', {})
  if not isinstance(progress, dict):
    progress = {}
  progress_html = render_progress(progress) if status == 'in_progress' else ''
  return f'''
<main class="content">
  <header class="model-header">
    <div><h1>{text(model_id)}</h1><span class="badge status-{attribute(status)}">{text(status.replace('_', ' '))}</span></div>
    <dl>
      <div><dt>Train start</dt><dd>{timestamp(summary.get('train_start'))}</dd></div>
      <div><dt>Train end</dt><dd>{timestamp(summary.get('train_end'))}</dd></div>
      <div><dt>Duration</dt><dd>{duration_between(summary.get('train_start'), summary.get('train_end'))}</dd></div>
    </dl>
  </header>
  {progress_html}
  {render_overview(summary, properties)}
  {render_training(properties.get('training'))}
  {render_quality(properties.get('metrics'))}
  {render_diagnostics(properties.get('metrics'))}
  {render_segments(model_id, segments, search, url_path)}
  {render_raw_files(model_id, properties.get('files', []), url_path)}
</main>'''


def render_progress(progress):
  completed = progress.get('completed_batches')
  total = progress.get('total_batches')
  percent = 0.0
  if isinstance(completed, (int, float)) and isinstance(total, (int, float)) and total:
    percent = min(100.0, max(0.0, completed * 100.0 / total))
  return f'''
<section class="progress-section">
  <div class="section-title"><h2>Training progress</h2><span>{percent:.1f}%</span></div>
  <div class="progress-track"><span style="width:{percent:.3f}%"></span></div>
  {render_key_values([
    ('Stage', progress.get('stage')),
    ('Epoch', progress.get('epoch')),
    ('Batches', str(completed) + ' / ' + str(total)),
    ('Elapsed', duration(progress.get('training_elapsed_seconds'))),
    ('Batch wait', duration(progress.get('batch_wait_seconds'))),
    ('Compute', duration(progress.get('training_compute_seconds'))),
  ])}
</section>'''


def render_overview(summary, properties):
  config = properties.get('config', {})
  training_summary = properties.get('training_summary', {})
  source = properties.get('source')
  model = config.get('model', {}) if isinstance(config, dict) else {}
  data = config.get('data', {}) if isinstance(config, dict) else {}
  training = config.get('training', {}) if isinstance(config, dict) else {}
  candidate_opening = config.get('candidate_opening', {}) if isinstance(config, dict) else {}
  values = [
    ('Candidates', model.get('candidates')),
    ('Extracted segments', summary.get('segments_count')),
    ('Aggregation', model.get('aggregation')),
    ('URL buckets', data.get('url_buckets')),
    ('Windows', ', '.join(map(str, data.get('windows_seconds', [])))),
    ('N values', ', '.join(map(str, data.get('n_values', [])))),
    ('Device', training.get('device')),
    ('Candidate opening', 'enabled' if candidate_opening.get('enabled') else 'disabled'),
    ('Candidate opening mode', candidate_opening.get('mode')),
    ('Epochs completed', training_summary.get('epochs_completed')),
    ('Best epoch', training_summary.get('best_epoch')),
    ('Best validation loss', training_summary.get('best_validation_loss')),
    ('Training rows', training_summary.get('training_rows')),
    ('Training CTR', training_summary.get('training_ctr')),
  ]
  if isinstance(source, dict):
    values.extend([
      ('Source rows', source.get('rows')),
      ('Source', source.get('name') or source.get('type')),
    ])
  return '<section><h2>Overview</h2>' + render_key_values(values) + '</section>'


def render_training(training):
  if not isinstance(training, list) or not training:
    return '<section><h2>Training</h2><p class="empty">Training history is not available.</p></section>'
  rows = ''.join(
    '<tr>'
    f'<td>{text(item.get("epoch"))}</td>'
    f'<td>{text(item.get("stage"))}</td>'
    f'<td>{text(_candidate_mask(item.get("candidate_opening")))}</td>'
    f'<td>{number(item.get("total"))}</td>'
    f'<td>{number(item.get("ctr"))}</td>'
    f'<td>{number(item.get("validation_loss"))}</td>'
    f'<td>{number(item.get("sparsity"))}</td>'
    f'<td>{number(item.get("binarization"))}</td>'
    f'<td>{number(item.get("url_duplicate"))}</td>'
    f'<td>{number(item.get("activation_duplicate"))}</td>'
    f'<td>{number(item.get("duplicate_regularization_scale"))}</td>'
    f'<td>{"yes" if item.get("best") else ""}</td>'
    '</tr>'
    for item in training
    if isinstance(item, dict))
  return f'''
<section><h2>Training</h2>{render_loss_chart(training)}<div class="table-scroll"><table>
  <thead><tr>
    <th>Epoch</th><th>Stage</th><th>Candidate mask</th><th>Total loss</th>
    <th>CTR loss</th><th>Validation</th>
    <th>Sparsity</th><th>Binarization</th><th>URL duplicate</th>
    <th>Activation duplicate</th><th>Duplicate scale</th><th>Best</th>
  </tr></thead>
  <tbody>{rows}</tbody>
</table></div></section>'''


def render_loss_chart(training):
  series = {
    'Total loss': [item.get('total') for item in training],
    'Validation': [item.get('validation_loss') for item in training],
  }
  finite_values = [
    float(value)
    for values in series.values()
    for value in values
    if isinstance(value, (int, float)) and math.isfinite(value)
  ]
  if not finite_values:
    return ''
  minimum = min(finite_values)
  maximum = max(finite_values)
  span = maximum - minimum or 1.0
  width = 900
  height = 240
  left = 64
  right = 20
  top = 20
  bottom = 36
  plot_width = width - left - right
  plot_height = height - top - bottom
  colors = ('#087f5b', '#c2410c')
  lines = []
  legend = []
  for index, (name, values) in enumerate(series.items()):
    points = []
    for item_index, value in enumerate(values):
      if not isinstance(value, (int, float)) or not math.isfinite(value):
        continue
      x = left + plot_width * item_index / max(1, len(values) - 1)
      y = top + plot_height * (maximum - value) / span
      points.append(f'{x:.2f},{y:.2f}')
    if points:
      lines.append(
        f'<polyline class="chart-line" fill="none" stroke="{colors[index]}" '
        f'stroke-width="2" points="{" ".join(points)}"/>')
    legend.append(
      f'<span><i style="background:{colors[index]}"></i>{text(name)}</span>')
  return f'''
<div class="chart-wrap">
  <svg class="chart" viewBox="0 0 {width} {height}" role="img" aria-label="Training loss chart">
    <line x1="{left}" y1="{top}" x2="{left}" y2="{height - bottom}" class="axis"/>
    <line x1="{left}" y1="{height - bottom}" x2="{width - right}" y2="{height - bottom}" class="axis"/>
    <text x="8" y="{top + 5}" class="axis-label">{number(maximum)}</text>
    <text x="8" y="{height - bottom}" class="axis-label">{number(minimum)}</text>
    {''.join(lines)}
  </svg>
  <div class="legend">{''.join(legend)}</div>
</div>'''


def render_quality(metrics):
  if not isinstance(metrics, dict) or not metrics:
    return '<section><h2>Soft versus hard</h2><p class="empty">Evaluation is not available.</p></section>'
  soft = metrics.get('soft_ctr', {})
  hard = metrics.get('hard_ctr', {})
  soft_hard = metrics.get('soft_hard', {})
  recovery = metrics.get('recovery', {})
  soft = soft if isinstance(soft, dict) else {}
  hard = hard if isinstance(hard, dict) else {}
  soft_hard = soft_hard if isinstance(soft_hard, dict) else {}
  recovery = recovery if isinstance(recovery, dict) else {}
  rows = ''.join(
    '<tr>'
    f'<td>{text(name)}</td>'
    f'<td>{number(soft.get(key))}</td>'
    f'<td>{number(hard.get(key))}</td>'
    '</tr>'
    for name, key in (
      ('Logloss', 'logloss'),
      ('ROC-AUC', 'roc_auc'),
      ('PR-AUC', 'pr_auc')))
  recovery_values = [
    ('Agreement rate', soft_hard.get('segment_agreement_rate')),
    ('Mean activation difference', soft_hard.get('mean_absolute_activation_difference')),
    ('URL precision', recovery.get('url_precision')),
    ('URL recall', recovery.get('url_recall')),
    ('URL F1', recovery.get('url_f1')),
    ('Window accuracy', recovery.get('window_accuracy')),
    ('N accuracy', recovery.get('n_accuracy')),
  ]
  return f'''
<section><h2>Soft versus hard</h2>
  <div class="quality-grid"><table><thead><tr><th>Metric</th><th>Soft</th><th>Hard</th></tr></thead><tbody>{rows}</tbody></table>
  {render_key_values(recovery_values)}</div>
</section>'''


def render_diagnostics(metrics):
  diagnostics = metrics.get('diagnostics', {}) if isinstance(metrics, dict) else {}
  if not isinstance(diagnostics, dict) or not diagnostics:
    return ''
  values = [
    ('Average URLs per segment', diagnostics.get('average_urls_per_extracted_segment')),
    ('Empty segments', diagnostics.get('empty_segments')),
    ('Highly similar pairs', diagnostics.get('highly_similar_segment_pairs')),
    ('URL gates near zero', diagnostics.get('url_gate_fraction_near_zero')),
    ('URL gates near one', diagnostics.get('url_gate_fraction_near_one')),
    ('Ambiguous URL gates', diagnostics.get('url_gate_fraction_ambiguous')),
  ]
  candidate_duplicates = diagnostics.get('candidate_duplicates', {})
  if not isinstance(candidate_duplicates, dict):
    candidate_duplicates = {}
  candidate_opening = diagnostics.get('candidate_opening', {})
  if not isinstance(candidate_opening, dict):
    candidate_opening = {}
  values.extend([
    ('Max URL Jaccard', candidate_duplicates.get('max_pairwise_url_jaccard')),
    ('Mean URL Jaccard', candidate_duplicates.get('mean_pairwise_url_jaccard')),
    ('Max activation similarity', candidate_duplicates.get('max_activation_similarity')),
    ('Mean activation similarity', candidate_duplicates.get('mean_activation_similarity')),
    ('URL pairs above 0.8', candidate_duplicates.get(
      'number_of_pairs_jaccard_above_0_8')),
    ('URL pairs above 0.95', candidate_duplicates.get(
      'number_of_pairs_jaccard_above_0_95')),
    ('Activation duplicate pairs', candidate_duplicates.get(
      'number_of_activation_duplicate_pairs')),
    ('Reseed duplicate pairs', candidate_duplicates.get(
      'number_of_reseed_duplicate_pairs')),
    ('Candidate mask', _candidate_mask(candidate_opening)),
    ('Active candidates', candidate_opening.get('active_candidates')),
    ('Joint fine-tune', candidate_opening.get('joint_finetune_active')),
  ])
  temperatures = diagnostics.get('temperatures')
  if isinstance(temperatures, dict):
    values.extend(
      ('Temperature ' + str(name), value)
      for name, value in temperatures.items())
  pairs = candidate_duplicates.get('most_similar_pairs', [])
  opening_rows = ''.join(
    '<tr>'
    f'<td>{text(candidate.get("index"))}</td>'
    f'<td>{"opened" if candidate.get("opened") else "closed"}</td>'
    f'<td>{text(candidate.get("epoch_opened"))}</td>'
    f'<td>{number(candidate.get("learning_rate_multiplier"))}</td>'
    f'<td>{text(", ".join(map(str, candidate.get("extracted_urls", []))))}</td>'
    f'<td>{text(_pair_urls(candidate.get("top_url_gates")))}</td>'
    f'<td>{number(candidate.get("forest_soft_importance"))}</td>'
    f'<td>{text(candidate.get("forest_hard_split_count"))}</td>'
    '</tr>'
    for candidate in candidate_opening.get('candidates', [])
    if isinstance(candidate, dict))
  opening_table = ''
  if opening_rows:
    opening_table = (
      '<h3>Candidate opening</h3><div class="table-scroll"><table><thead><tr>'
      '<th>Candidate</th><th>State</th><th>Opened epoch</th><th>LR multiplier</th>'
      '<th>Extracted URLs</th><th>Top URL gates</th><th>Soft importance</th>'
      '<th>Hard splits</th></tr></thead><tbody>' + opening_rows + '</tbody></table></div>')
  pair_rows = ''.join(
    '<tr>'
    f'<td>{text(pair.get("candidate_i"))} / {text(pair.get("candidate_j"))}</td>'
    f'<td>{number(pair.get("url_jaccard"))}</td>'
    f'<td>{number(pair.get("activation_similarity"))}</td>'
    f'<td>{number(pair.get("forest_importance_i"))} / '
    f'{number(pair.get("forest_importance_j"))}</td>'
    f'<td>{text(_pair_urls(pair.get("top_urls_i")))} / '
    f'{text(_pair_urls(pair.get("top_urls_j")))}</td>'
    '</tr>'
    for pair in pairs
    if isinstance(pair, dict))
  pair_table = ''
  if pair_rows:
    pair_table = (
      '<div class="table-scroll"><table><thead><tr><th>Candidates</th>'
      '<th>URL Jaccard</th><th>Activation similarity</th><th>Forest importance</th>'
      '<th>Top URLs</th></tr></thead><tbody>' + pair_rows + '</tbody></table></div>')
  return (
    '<section><h2>Diagnostics</h2>' + render_key_values(values) + opening_table +
    pair_table + '</section>')


def _pair_urls(values):
  if not isinstance(values, list):
    return ''
  return ', '.join(
    str(value.get('url'))
    for value in values
    if isinstance(value, dict))


def _candidate_mask(value):
  if not isinstance(value, dict):
    return ''
  mask = value.get('candidate_mask', [])
  if not isinstance(mask, list):
    return ''
  return ''.join(str(int(bool(item))) for item in mask)


def render_segments(model_id, result, search, url_path):
  if result is None:
    return ''
  rows = ''.join(
    render_segment_row(model_id, segment, url_path)
    for segment in result.get('items', []))
  action = application_url(url_path)
  notice = ''
  if result.get('total', 0) > len(result.get('items', [])):
    notice = '<p class="notice">Showing first 200 matching segments.</p>'
  return f'''
<section><div class="section-title"><h2>Segments</h2><span>{result.get('total', 0)}</span></div>
  <form class="filter" method="get" action="{attribute(action)}">
    <input type="hidden" name="model" value="{attribute(model_id)}">
    <input type="search" name="search" value="{attribute(search or '')}" placeholder="URL, channel or relation">
    <button type="submit">Filter</button>
  </form>
  <div class="table-scroll"><table>
    <thead><tr><th>ID</th><th>Window</th><th>N</th><th>URLs</th><th>Activation</th><th>Forest splits</th><th>Relations</th><th>Examples</th></tr></thead>
    <tbody>{rows or '<tr><td colspan="8">No matching segments</td></tr>'}</tbody>
  </table></div>{notice}
</section>'''


def render_segment_row(model_id, segment, url_path):
  segment_id = segment.get('segment_id')
  link = application_url(
    url_path,
    '/models/' + urllib.parse.quote(model_id) + '/segments/' +
    urllib.parse.quote(str(segment_id)) + '/view')
  urls = segment.get('urls', [])
  examples = ', '.join(str(url) for url in urls[:3])
  return (
    '<tr>'
    f'<td><a href="{attribute(link)}">{text(segment_id)}</a></td>'
    f'<td>{duration(segment.get("window_seconds"))}</td>'
    f'<td>{text(segment.get("min_visits"))}</td>'
    f'<td>{len(urls)}</td>'
    f'<td>{number(segment.get("average_activation"))}</td>'
    f'<td>{text(segment.get("forest_split_count"))}</td>'
    f'<td>{len(segment.get("relations", []))}</td>'
    f'<td class="wide mono">{text(examples)}</td>'
    '</tr>')


def render_segment_page(model_id, segment, url_path='/'):
  relation_rows = ''.join(
    '<tr>'
    f'<td>{text(relation.get("channel_id"))}</td>'
    f'<td>{text(relation.get("relation"))}</td>'
    f'<td>{number(relation.get("candidate_given_channel"))}</td>'
    f'<td>{number(relation.get("channel_given_candidate"))}</td>'
    '</tr>'
    for relation in segment.get('relations', [])
    if isinstance(relation, dict))
  url_rows = ''.join(
    f'<tr><td>{index + 1}</td><td class="wide mono">{text(url)}</td></tr>'
    for index, url in enumerate(segment.get('urls', [])))
  back_url = application_url(url_path, '?model=' + urllib.parse.quote(model_id))
  return '<!doctype html>' + f'''
<html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Segment {text(segment.get('segment_id'))}</title><style>{STYLES}</style></head>
<body><main class="standalone">
  <a class="back" href="{attribute(back_url)}">Back to model</a>
  <h1>Segment {text(segment.get('segment_id'))}</h1>
  {render_key_values([
    ('Window', duration(segment.get('window_seconds'))),
    ('Minimum visits', segment.get('min_visits')),
    ('URLs', len(segment.get('urls', []))),
    ('Forest splits', segment.get('forest_split_count')),
    ('Average activation', segment.get('average_activation')),
  ])}
  <section><h2>URLs</h2><div class="table-scroll"><table>
    <thead><tr><th>#</th><th>URL</th></tr></thead><tbody>{url_rows}</tbody>
  </table></div></section>
  <section><h2>Relations</h2><div class="table-scroll"><table>
    <thead><tr><th>Channel</th><th>Relation</th><th>P(candidate | channel)</th><th>P(channel | candidate)</th></tr></thead>
    <tbody>{relation_rows or '<tr><td colspan="4">No relations</td></tr>'}</tbody>
  </table></div></section>
</main></body></html>'''


def render_raw_files(model_id, files, url_path):
  links = []
  for file_name in files:
    url = application_url(
      url_path,
      '/models/' + urllib.parse.quote(model_id) + '/files/' + file_name)
    links.append(f'<a href="{attribute(url)}">{text(file_name)}</a>')
  return '<section><h2>Artifacts</h2><div class="artifact-links">' + ''.join(links) + '</div></section>'


def render_key_values(values):
  return '<dl class="properties">' + ''.join(
    f'<div><dt>{text(name)}</dt><dd>{format_value(value)}</dd></div>'
    for name, value in values
    if value is not None and value != '') + '</dl>'


def normalize_url_path(url_path):
  if not url_path or url_path == '/':
    return '/'
  return '/' + url_path.strip('/')


def application_url(url_path, path=''):
  base = normalize_url_path(url_path)
  if path.startswith('?'):
    return (base if base != '/' else '/') + path
  if not path:
    return base
  return ('' if base == '/' else base) + '/' + path.lstrip('/')


def text(value):
  return html.escape('' if value is None else str(value))


def attribute(value):
  return html.escape('' if value is None else str(value), quote=True)


def format_value(value):
  if isinstance(value, (float, int)) and not isinstance(value, bool):
    return number(value)
  return text(value)


def number(value):
  if value is None:
    return ''
  if isinstance(value, bool):
    return 'yes' if value else 'no'
  if isinstance(value, int):
    return str(value)
  try:
    numeric = float(value)
  except (TypeError, ValueError):
    return text(value)
  if not math.isfinite(numeric):
    return text(value)
  return f'{numeric:.8g}'


def timestamp(value):
  if not value:
    return ''
  return text(str(value).replace('T', ' ').removesuffix('Z'))


def duration(value):
  if value is None:
    return ''
  try:
    seconds = float(value)
  except (TypeError, ValueError):
    return text(value)
  if seconds < 60:
    return f'{seconds:.3g} s'
  if seconds < 3600:
    return f'{seconds / 60:.3g} min'
  if seconds < 86400:
    return f'{seconds / 3600:.3g} h'
  return f'{seconds / 86400:.3g} d'


def duration_between(start, end):
  if not start or not end:
    return ''
  try:
    import datetime
    start_value = datetime.datetime.fromisoformat(str(start).replace('Z', '+00:00'))
    end_value = datetime.datetime.fromisoformat(str(end).replace('Z', '+00:00'))
    return duration((end_value - start_value).total_seconds())
  except (TypeError, ValueError):
    return ''


STYLES = '''
:root { color-scheme: light; font-family: Inter, system-ui, sans-serif; color: #202124; background: #f4f5f6; }
* { box-sizing: border-box; letter-spacing: 0; }
body { margin: 0; font-size: 14px; }
a { color: #0969a2; text-decoration: none; }
a:hover { text-decoration: underline; }
.layout { min-height: 100vh; display: grid; grid-template-columns: 270px minmax(0, 1fr); }
.sidebar { background: #202428; color: #f8f9fa; border-right: 1px solid #111; }
.sidebar header { height: 52px; display: flex; align-items: center; justify-content: space-between; padding: 0 16px; border-bottom: 1px solid #3a3f44; }
.sidebar header span { color: #adb5bd; font-variant-numeric: tabular-nums; }
.sidebar nav { position: sticky; top: 0; max-height: 100vh; overflow-y: auto; }
.model-link { position: relative; display: grid; grid-template-columns: 1fr auto; gap: 4px 8px; min-height: 62px; padding: 10px 14px 10px 18px; color: #f1f3f5; border-bottom: 1px solid #343a40; }
.model-link::before { content: ''; position: absolute; left: 0; top: 0; bottom: 0; width: 4px; background: #868e96; }
.model-link.status-published::before { background: #2f9e44; }
.model-link.status-in_progress::before { background: #f59f00; }
.model-link.status-interrupted::before { background: #e03131; }
.model-link.selected { background: #343a40; }
.model-link:hover { background: #2b3035; text-decoration: none; }
.model-id { font-weight: 650; }
.model-status { color: #ced4da; font-size: 12px; text-transform: uppercase; }
.model-link small { grid-column: 1 / -1; color: #adb5bd; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }
.content, .standalone, .empty-state { width: 100%; max-width: 1500px; padding: 24px 32px 56px; margin: 0 auto; background: #fff; }
.standalone { max-width: 1200px; background: white; min-height: 100vh; }
.model-header { display: flex; align-items: flex-start; justify-content: space-between; gap: 24px; padding-bottom: 22px; }
h1 { margin: 0 0 8px; font-size: 27px; line-height: 1.2; }
h2 { margin: 0 0 16px; font-size: 18px; }
.badge { display: inline-block; padding: 3px 7px; border: 1px solid #adb5bd; border-radius: 3px; font-size: 12px; text-transform: uppercase; background: #fff; }
.badge.status-published { color: #237a35; border-color: #69b578; }
.badge.status-in_progress { color: #9c6500; border-color: #e9b949; }
.badge.status-interrupted { color: #b02525; border-color: #dc7777; }
.model-header > dl { display: flex; margin: 0; gap: 28px; }
.model-header dl div { min-width: 120px; }
dt { color: #697077; font-size: 12px; text-transform: uppercase; }
dd { margin: 4px 0 0; font-variant-numeric: tabular-nums; }
section { border-top: 1px solid #ced4da; margin: 0; padding: 22px 0; }
.progress-section { border-top-color: #d99000; }
.section-title { display: flex; align-items: baseline; justify-content: space-between; gap: 12px; }
.section-title span { color: #697077; font-variant-numeric: tabular-nums; }
.progress-track { height: 8px; background: #e9ecef; margin-bottom: 18px; overflow: hidden; }
.progress-track span { display: block; height: 100%; background: #d99000; }
.properties { display: grid; grid-template-columns: repeat(auto-fit, minmax(155px, 1fr)); gap: 16px 24px; margin: 0; }
.properties div { min-width: 0; }
.properties dd { overflow-wrap: anywhere; }
.quality-grid { display: grid; grid-template-columns: minmax(360px, .8fr) minmax(420px, 1.2fr); gap: 28px; align-items: start; }
.table-scroll { overflow-x: auto; }
table { width: 100%; border-collapse: collapse; font-variant-numeric: tabular-nums; }
th, td { padding: 8px 10px; border-bottom: 1px solid #dee2e6; text-align: left; vertical-align: top; white-space: nowrap; }
th { background: #f1f3f5; color: #4c535a; font-size: 12px; text-transform: uppercase; }
td.wide { max-width: 440px; overflow: hidden; text-overflow: ellipsis; }
.mono { font-family: ui-monospace, SFMono-Regular, Menlo, monospace; font-size: 12px; }
.chart-wrap { margin-bottom: 20px; }
.chart { display: block; width: 100%; height: 240px; background: #fbfcfc; border: 1px solid #dee2e6; }
.axis { stroke: #868e96; stroke-width: 1; }
.axis-label { font-size: 11px; fill: #697077; }
.legend { display: flex; gap: 18px; margin-top: 8px; color: #4c535a; }
.legend span { display: flex; align-items: center; gap: 6px; }
.legend i { width: 16px; height: 3px; display: inline-block; }
.filter { display: flex; gap: 8px; margin: 0 0 14px; max-width: 560px; }
.filter input { min-width: 0; flex: 1; height: 34px; border: 1px solid #adb5bd; padding: 0 9px; }
.filter button { height: 34px; border: 1px solid #495057; background: #495057; color: white; padding: 0 14px; cursor: pointer; }
.artifact-links { display: flex; flex-wrap: wrap; gap: 8px 18px; }
.empty, .notice { color: #697077; }
.empty-state { background: #fff; }
.back { display: inline-block; margin-bottom: 18px; }
@media (max-width: 900px) {
  .layout { grid-template-columns: 1fr; }
  .sidebar nav { position: static; max-height: 260px; }
  .content, .standalone, .empty-state { padding: 18px 14px 40px; }
  .model-header, .model-header > dl { display: block; }
  .model-header dl div { margin-top: 12px; }
  .quality-grid { grid-template-columns: 1fr; }
}
'''
