SECTION_SPECS = (
  ('processing_steps', 'Processing steps', 'train_steps', 'steps'),
  ('properties', 'Properties', 'properties', 'items'),
  ('feature_groups', 'Feature groups', 'feature_groups', 'groups'),
  ('datasets', 'Datasets', 'dataset_sizes', 'datasets'),
  (
    'ctr_thresholds',
    'CTR threshold checking',
    'ctr_thresholds',
    'thresholds',
  ),
  ('training_report', 'Training report', 'logloss_history', 'history'),
  (
    'feature_importance',
    'Feature importance',
    'features_importance',
    'features',
  ),
  (
    'post_processing_results',
    'Campaign evaluations',
    'targets',
    'targets',
  ),
)


SECTION_SPECS_BY_ID = {
  section_id: (title, field, data_field)
  for section_id, title, field, data_field in SECTION_SPECS
}


def traits_sections(traits):
  if not isinstance(traits, dict):
    return []

  sections = []
  section_ids = set()
  source_sections = traits.get('sections')
  if isinstance(source_sections, list):
    for section in source_sections:
      if not isinstance(section, dict):
        continue
      section_id = section.get('id')
      if not isinstance(section_id, str) or not section_id:
        continue
      sections.append(dict(section))
      section_ids.add(section_id)

  for section_id, title, field, data_field in SECTION_SPECS:
    if section_id in section_ids or field not in traits:
      continue
    sections.append({
      'id': section_id,
      'title': title,
      'data': {data_field: traits[field]},
    })
  return sections


def traits_with_sections(traits):
  if not isinstance(traits, dict):
    return traits

  result = dict(traits)
  sections = traits_sections(result)
  for _, _, field, _ in SECTION_SPECS:
    result.pop(field, None)
  if sections or 'sections' in result:
    try:
      artifact_version = int(result.get('artifact_version', 1))
    except (TypeError, ValueError):
      artifact_version = 1
    result['artifact_version'] = max(
      artifact_version,
      2,
    )
    result['sections'] = sections
  return result


def section_value(traits, section_id, default=None):
  specification = SECTION_SPECS_BY_ID.get(section_id)
  if specification is None:
    return default
  _, legacy_field, data_field = specification
  for section in traits_sections(traits):
    if section.get('id') != section_id:
      continue
    data = section.get('data')
    if isinstance(data, dict) and data_field in data:
      return data[data_field]
    return default
  if isinstance(traits, dict):
    return traits.get(legacy_field, default)
  return default
