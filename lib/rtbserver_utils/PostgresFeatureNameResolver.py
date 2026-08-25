import re

import psycopg2


class PostgresFeatureNameResolver:
  DOMAIN_ENTITY = {
    'advertiser': 'account',
    'advertiser_id': 'account',
    'campaign': 'campaign',
    'campaign_id': 'campaign',
    'ccg': 'ccg',
    'ccg_id': 'ccg',
    'cc_id': 'ccid',
    'ccid': 'ccid',
    'channel': 'channel',
    'channel_id': 'channel',
    'colo': 'colo',
    'colo_id': 'colo',
    'creative': 'creative',
    'creative_id': 'creative',
    'device': 'channel',
    'geochannel': 'geochannel',
    'isp': 'account',
    'publisher': 'account',
    'publisher_id': 'account',
    'site': 'site',
    'site_id': 'site',
    'size': 'size',
    'size_id': 'size',
    'sizeid': 'size',
    'tag': 'tag',
    'tag_id': 'tag',
  }

  ENTITY_QUERIES = {
    'account': '''
      SELECT account_id, name, name
      FROM account
      WHERE account_id = ANY(%s)
    ''',
    'campaign': '''
      SELECT campaign.campaign_id, account.name, campaign.name
      FROM campaign
      JOIN account USING (account_id)
      WHERE campaign.campaign_id = ANY(%s)
    ''',
    'ccg': '''
      SELECT ccg.ccg_id, account.name, ccg.name
      FROM campaigncreativegroup AS ccg
      JOIN campaign USING (campaign_id)
      JOIN account USING (account_id)
      WHERE ccg.ccg_id = ANY(%s)
    ''',
    'ccid': '''
      SELECT cc.cc_id, account.name, creative.name
      FROM campaigncreative AS cc
      JOIN creative USING (creative_id)
      JOIN account USING (account_id)
      WHERE cc.cc_id = ANY(%s)
    ''',
    'channel': '''
      SELECT channel.channel_id, account.name, channel.name
      FROM channel
      LEFT JOIN account USING (account_id)
      WHERE channel.channel_id = ANY(%s)
    ''',
    'geochannel': '''
      WITH RECURSIVE geochannel_paths AS (
        SELECT
          channel.channel_id AS entity_id,
          channel.parent_channel_id,
          ARRAY[channel.name::text] AS names
        FROM channel
        WHERE channel.channel_id = ANY(%s)

        UNION ALL

        SELECT
          geochannel_paths.entity_id,
          parent.parent_channel_id,
          ARRAY[parent.name::text] || geochannel_paths.names
        FROM geochannel_paths
        JOIN channel AS parent
          ON parent.channel_id = geochannel_paths.parent_channel_id
      )
      SELECT
        entity_id,
        NULL::text,
        array_to_string(names, '/')
      FROM geochannel_paths
      WHERE parent_channel_id IS NULL
    ''',
    'colo': '''
      SELECT colocation.colo_id, account.name, colocation.name
      FROM colocation
      JOIN account USING (account_id)
      WHERE colocation.colo_id = ANY(%s)
    ''',
    'creative': '''
      SELECT creative.creative_id, account.name, creative.name
      FROM creative
      JOIN account USING (account_id)
      WHERE creative.creative_id = ANY(%s)
    ''',
    'site': '''
      SELECT site.site_id, account.name, site.name
      FROM site
      JOIN account USING (account_id)
      WHERE site.site_id = ANY(%s)
    ''',
    'size': '''
      SELECT size_id, NULL::text, name
      FROM creativesize
      WHERE size_id = ANY(%s)
    ''',
    'tag': '''
      SELECT tags.tag_id, account.name, tags.name
      FROM tags
      JOIN site USING (site_id)
      JOIN account USING (account_id)
      WHERE tags.tag_id = ANY(%s)
    ''',
  }

  FEATURE_PART_RE = re.compile(r'(?:^|,)([a-z_]+):([^,]+)')

  def __init__(self, connection_string):
    if not connection_string:
      raise ValueError('PostgreSQL connection string is required')
    self.connection_string = connection_string

  def resolve(self, features):
    feature_parts = {
      feature: self.feature_entity_parts_(feature)
      for feature in features
    }
    entity_ids = {}
    for parts in feature_parts.values():
      for entity, entity_id in parts:
        entity_ids.setdefault(entity, set()).add(entity_id)

    entity_names = {}
    if entity_ids:
      with psycopg2.connect(self.connection_string) as connection:
        with connection.cursor() as cursor:
          for entity, ids in entity_ids.items():
            cursor.execute(self.ENTITY_QUERIES[entity], (sorted(ids),))
            for entity_id, account_name, entity_name in cursor.fetchall():
              if entity_name is None:
                continue
              if account_name is None:
                name = str(entity_name)
              else:
                name = str(account_name) + '/' + str(entity_name)
              entity_names[(entity, entity_id)] = name

    result = {}
    for feature, parts in feature_parts.items():
      names = []
      for part in parts:
        name = entity_names.get(part)
        if name is not None and name not in names:
          names.append(name)
      if names:
        result[feature] = ', '.join(names)
    return result

  @classmethod
  def feature_entity_parts_(cls, feature):
    result = []
    for match in cls.FEATURE_PART_RE.finditer(feature):
      entity = cls.DOMAIN_ENTITY.get(match.group(1))
      if entity is None:
        continue
      try:
        entity_id = int(match.group(2))
      except ValueError:
        continue
      result.append((entity, entity_id))
    return result
