\set ON_ERROR_STOP on
BEGIN;

DO $configure$
BEGIN
  PERFORM set_config(
    'foros.geo_channel_merge_plan',
    $geo_plan$[{"merge_no":1,"keep_region":"Khakass","keep_name":"Abaza","keep_latitude":"52.655000","keep_longitude":"90.092800","delete_region":"Khakass","delete_name":"abaza","delete_latitude":"52.650000","delete_longitude":"90.090000"},{"merge_no":2,"keep_region":"Permskiy Kray","keep_name":"Berezniki","keep_latitude":"59.415200","keep_longitude":"56.812400","delete_region":"Permskiy Kray","delete_name":"Beresniki","delete_latitude":"59.409100","delete_longitude":"56.820400"},{"merge_no":3,"keep_region":"Khabarovsk","keep_name":"Bogorodskaya","keep_latitude":"52.365800","keep_longitude":"140.445600","delete_region":"Khabarovsk","delete_name":"Bogorodskoye","delete_latitude":"52.370900","delete_longitude":"140.442100"},{"merge_no":4,"keep_region":"Orenburg","keep_name":"Buzuluk","keep_latitude":"52.780700","keep_longitude":"52.263500","delete_region":"Orenburg","delete_name":"buzuluk","delete_latitude":"52.783300","delete_longitude":"52.250000"},{"merge_no":5,"keep_region":"Samara","keep_name":"otradnyy","keep_latitude":"53.366700","keep_longitude":"51.350000","delete_region":"Samara","delete_name":"Otradny","delete_latitude":"53.376000","delete_longitude":"51.345200"},{"merge_no":6,"keep_region":"Astrakhan'","keep_name":"Kapustin Yar-1","keep_latitude":"48.584200","keep_longitude":"45.733800","delete_region":"Astrakhan'","delete_name":"znamensk","delete_latitude":"48.583300","delete_longitude":"45.750000"},{"merge_no":7,"keep_region":"Sakhalin","keep_name":"smirnykh","keep_latitude":"49.750000","keep_longitude":"142.833300","delete_region":"Sakhalin","delete_name":"Smirnykh","delete_latitude":"49.746000","delete_longitude":"142.837200"},{"merge_no":8,"keep_region":"Moscow City","keep_name":"Poselok Tekstilshchiki","keep_latitude":"55.700000","keep_longitude":"37.733300","delete_region":"Moscow City","delete_name":"Poselok Tekstilshchiki","delete_latitude":"55.700300","delete_longitude":"37.742700"},{"merge_no":9,"keep_region":"Orenburg","keep_name":"Saraktash","keep_latitude":"51.787700","keep_longitude":"56.360900","delete_region":"Orenburg","delete_name":"saraktash'","delete_latitude":"51.787700","delete_longitude":"56.360900"},{"merge_no":10,"keep_region":"Irkutsk","keep_name":"Ust-ilimsk","keep_latitude":"58.000600","keep_longitude":"102.661900","delete_region":"Irkutsk","delete_name":"Ust-ilim","delete_latitude":"58.008000","delete_longitude":"102.669300"},{"merge_no":11,"keep_region":"Kamchatskiy Kray","keep_name":"Yelizovo","keep_latitude":"53.183300","keep_longitude":"158.383300","delete_region":"Kamchatskiy Kray","delete_name":"Elizovo","delete_latitude":"53.189100","delete_longitude":"158.381300"},{"merge_no":12,"keep_region":"Zabaykal'skiy Kray","keep_name":"Zabaykalsk","keep_latitude":"49.633300","keep_longitude":"117.316700","delete_region":"Zabaykal'skiy Kray","delete_name":"Zabaykalsk","delete_latitude":"49.651300","delete_longitude":"117.325600"},{"merge_no":13,"keep_region":"Krym","keep_name":"Feodosia","keep_latitude":"45.036800","keep_longitude":"35.377900","delete_region":"Krym","delete_name":"Theodosia","delete_latitude":"45.036800","delete_longitude":"35.377900"},{"merge_no":14,"keep_region":"Zabaykal'skiy Kray","keep_name":"Mikhailovsk","keep_latitude":"51.113700","keep_longitude":"119.434500","delete_region":"Zabaykal'skiy Kray","delete_name":"Mikhailovsk","delete_latitude":"51.116700","delete_longitude":"119.433300"},{"merge_no":15,"keep_region":"Nizhegorod","keep_name":"yuganets","keep_latitude":"56.250000","keep_longitude":"43.236100","delete_region":"Nizhegorod","delete_name":"Yuganets","delete_latitude":"56.250800","delete_longitude":"43.230700"},{"merge_no":16,"keep_region":"Krasnoyarskiy Kray","keep_name":"Yeniseisk","keep_latitude":"58.449400","keep_longitude":"92.179700","delete_region":"Krasnoyarskiy Kray","delete_name":"Yeniseysk","delete_latitude":"58.449700","delete_longitude":"92.170300"},{"merge_no":17,"keep_region":"Krasnoyarskiy Kray","keep_name":"Krasnoyarsk-45","keep_latitude":"56.112400","keep_longitude":"94.598500","delete_region":"Krasnoyarskiy Kray","delete_name":"Zelenogorsk","delete_latitude":"56.110300","delete_longitude":"94.571600"}]$geo_plan$,
    true);
END
$configure$;

DO $validation$
DECLARE invalid_rows integer;
BEGIN
  WITH spec AS (
  SELECT *
  FROM jsonb_to_recordset(
    current_setting('foros.geo_channel_merge_plan')::jsonb)
  AS value(
    merge_no integer,
    keep_region text,
    keep_name text,
    keep_latitude numeric,
    keep_longitude numeric,
    delete_region text,
    delete_name text,
    delete_latitude numeric,
    delete_longitude numeric)
), matches AS (
  SELECT s.merge_no, role.role,
         array_agg(c.channel_id ORDER BY c.channel_id)
           FILTER (WHERE c.channel_id IS NOT NULL) AS channel_ids
  FROM spec s
  CROSS JOIN LATERAL (VALUES
    ('keep', s.keep_region, s.keep_name,
      s.keep_latitude, s.keep_longitude),
    ('delete', s.delete_region, s.delete_name,
      s.delete_latitude, s.delete_longitude)
  ) role(role, region_name, channel_name, latitude, longitude)
  LEFT JOIN LATERAL (
    SELECT channel.channel_id
    FROM channel
    JOIN channel parent ON parent.channel_id = channel.parent_channel_id
    WHERE channel.country_code = 'RU'
      AND channel.channel_type = 'G'
      AND channel.geo_type = 'CITY'
      AND channel.name = role.channel_name
      AND abs(channel.latitude - role.latitude) <= 0.000001
      AND abs(channel.longitude - role.longitude) <= 0.000001
      AND parent.name = role.region_name
  ) c ON true
  GROUP BY s.merge_no, role.role
), resolved AS (
  SELECT merge_no,
         max(channel_ids) FILTER (WHERE role = 'keep') keep_ids,
         max(channel_ids) FILTER (WHERE role = 'delete') delete_ids
  FROM matches
  GROUP BY merge_no
), plan AS (
  SELECT merge_no, keep_ids[1] keep_channel_id,
         delete_ids[1] delete_channel_id
  FROM resolved
  WHERE coalesce(cardinality(keep_ids), 0) = 1
    AND coalesce(cardinality(delete_ids), 0) = 1
)
  SELECT count(*) INTO invalid_rows
  FROM resolved
  WHERE coalesce(cardinality(keep_ids), 0) <> 1
     OR coalesce(cardinality(delete_ids), 0) <> 1;
  IF invalid_rows <> 0 THEN
    RAISE EXCEPTION 'each natural channel key must resolve to exactly one row';
  END IF;

  WITH spec AS (
  SELECT *
  FROM jsonb_to_recordset(
    current_setting('foros.geo_channel_merge_plan')::jsonb)
  AS value(
    merge_no integer,
    keep_region text,
    keep_name text,
    keep_latitude numeric,
    keep_longitude numeric,
    delete_region text,
    delete_name text,
    delete_latitude numeric,
    delete_longitude numeric)
), matches AS (
  SELECT s.merge_no, role.role,
         array_agg(c.channel_id ORDER BY c.channel_id)
           FILTER (WHERE c.channel_id IS NOT NULL) AS channel_ids
  FROM spec s
  CROSS JOIN LATERAL (VALUES
    ('keep', s.keep_region, s.keep_name,
      s.keep_latitude, s.keep_longitude),
    ('delete', s.delete_region, s.delete_name,
      s.delete_latitude, s.delete_longitude)
  ) role(role, region_name, channel_name, latitude, longitude)
  LEFT JOIN LATERAL (
    SELECT channel.channel_id
    FROM channel
    JOIN channel parent ON parent.channel_id = channel.parent_channel_id
    WHERE channel.country_code = 'RU'
      AND channel.channel_type = 'G'
      AND channel.geo_type = 'CITY'
      AND channel.name = role.channel_name
      AND abs(channel.latitude - role.latitude) <= 0.000001
      AND abs(channel.longitude - role.longitude) <= 0.000001
      AND parent.name = role.region_name
  ) c ON true
  GROUP BY s.merge_no, role.role
), resolved AS (
  SELECT merge_no,
         max(channel_ids) FILTER (WHERE role = 'keep') keep_ids,
         max(channel_ids) FILTER (WHERE role = 'delete') delete_ids
  FROM matches
  GROUP BY merge_no
), plan AS (
  SELECT merge_no, keep_ids[1] keep_channel_id,
         delete_ids[1] delete_channel_id
  FROM resolved
  WHERE coalesce(cardinality(keep_ids), 0) = 1
    AND coalesce(cardinality(delete_ids), 0) = 1
)
  SELECT count(*) INTO invalid_rows
  FROM (
    SELECT delete_channel_id
    FROM plan
    GROUP BY delete_channel_id
    HAVING count(*) <> 1
  ) duplicated;
  IF invalid_rows <> 0 THEN
    RAISE EXCEPTION 'one channel is deleted by multiple plan rows';
  END IF;

  WITH spec AS (
  SELECT *
  FROM jsonb_to_recordset(
    current_setting('foros.geo_channel_merge_plan')::jsonb)
  AS value(
    merge_no integer,
    keep_region text,
    keep_name text,
    keep_latitude numeric,
    keep_longitude numeric,
    delete_region text,
    delete_name text,
    delete_latitude numeric,
    delete_longitude numeric)
), matches AS (
  SELECT s.merge_no, role.role,
         array_agg(c.channel_id ORDER BY c.channel_id)
           FILTER (WHERE c.channel_id IS NOT NULL) AS channel_ids
  FROM spec s
  CROSS JOIN LATERAL (VALUES
    ('keep', s.keep_region, s.keep_name,
      s.keep_latitude, s.keep_longitude),
    ('delete', s.delete_region, s.delete_name,
      s.delete_latitude, s.delete_longitude)
  ) role(role, region_name, channel_name, latitude, longitude)
  LEFT JOIN LATERAL (
    SELECT channel.channel_id
    FROM channel
    JOIN channel parent ON parent.channel_id = channel.parent_channel_id
    WHERE channel.country_code = 'RU'
      AND channel.channel_type = 'G'
      AND channel.geo_type = 'CITY'
      AND channel.name = role.channel_name
      AND abs(channel.latitude - role.latitude) <= 0.000001
      AND abs(channel.longitude - role.longitude) <= 0.000001
      AND parent.name = role.region_name
  ) c ON true
  GROUP BY s.merge_no, role.role
), resolved AS (
  SELECT merge_no,
         max(channel_ids) FILTER (WHERE role = 'keep') keep_ids,
         max(channel_ids) FILTER (WHERE role = 'delete') delete_ids
  FROM matches
  GROUP BY merge_no
), plan AS (
  SELECT merge_no, keep_ids[1] keep_channel_id,
         delete_ids[1] delete_channel_id
  FROM resolved
  WHERE coalesce(cardinality(keep_ids), 0) = 1
    AND coalesce(cardinality(delete_ids), 0) = 1
)
  SELECT count(*) INTO invalid_rows
  FROM plan p
  JOIN channel keep ON keep.channel_id = p.keep_channel_id
  JOIN channel deleted ON deleted.channel_id = p.delete_channel_id
  WHERE keep.status = 'D'
     OR deleted.status NOT IN ('A', 'D');
  IF invalid_rows <> 0 THEN
    RAISE EXCEPTION 'unexpected channel status in merge plan';
  END IF;

  WITH spec AS (
  SELECT *
  FROM jsonb_to_recordset(
    current_setting('foros.geo_channel_merge_plan')::jsonb)
  AS value(
    merge_no integer,
    keep_region text,
    keep_name text,
    keep_latitude numeric,
    keep_longitude numeric,
    delete_region text,
    delete_name text,
    delete_latitude numeric,
    delete_longitude numeric)
), matches AS (
  SELECT s.merge_no, role.role,
         array_agg(c.channel_id ORDER BY c.channel_id)
           FILTER (WHERE c.channel_id IS NOT NULL) AS channel_ids
  FROM spec s
  CROSS JOIN LATERAL (VALUES
    ('keep', s.keep_region, s.keep_name,
      s.keep_latitude, s.keep_longitude),
    ('delete', s.delete_region, s.delete_name,
      s.delete_latitude, s.delete_longitude)
  ) role(role, region_name, channel_name, latitude, longitude)
  LEFT JOIN LATERAL (
    SELECT channel.channel_id
    FROM channel
    JOIN channel parent ON parent.channel_id = channel.parent_channel_id
    WHERE channel.country_code = 'RU'
      AND channel.channel_type = 'G'
      AND channel.geo_type = 'CITY'
      AND channel.name = role.channel_name
      AND abs(channel.latitude - role.latitude) <= 0.000001
      AND abs(channel.longitude - role.longitude) <= 0.000001
      AND parent.name = role.region_name
  ) c ON true
  GROUP BY s.merge_no, role.role
), resolved AS (
  SELECT merge_no,
         max(channel_ids) FILTER (WHERE role = 'keep') keep_ids,
         max(channel_ids) FILTER (WHERE role = 'delete') delete_ids
  FROM matches
  GROUP BY merge_no
), plan AS (
  SELECT merge_no, keep_ids[1] keep_channel_id,
         delete_ids[1] delete_channel_id
  FROM resolved
  WHERE coalesce(cardinality(keep_ids), 0) = 1
    AND coalesce(cardinality(delete_ids), 0) = 1
)
  SELECT count(*) INTO invalid_rows
  FROM plan p
  JOIN plan other ON other.delete_channel_id = p.keep_channel_id;
  IF invalid_rows <> 0 THEN
    RAISE EXCEPTION 'a channel cannot be both survivor and deleted';
  END IF;

  IF EXISTS (
    WITH spec AS (
  SELECT *
  FROM jsonb_to_recordset(
    current_setting('foros.geo_channel_merge_plan')::jsonb)
  AS value(
    merge_no integer,
    keep_region text,
    keep_name text,
    keep_latitude numeric,
    keep_longitude numeric,
    delete_region text,
    delete_name text,
    delete_latitude numeric,
    delete_longitude numeric)
), matches AS (
  SELECT s.merge_no, role.role,
         array_agg(c.channel_id ORDER BY c.channel_id)
           FILTER (WHERE c.channel_id IS NOT NULL) AS channel_ids
  FROM spec s
  CROSS JOIN LATERAL (VALUES
    ('keep', s.keep_region, s.keep_name,
      s.keep_latitude, s.keep_longitude),
    ('delete', s.delete_region, s.delete_name,
      s.delete_latitude, s.delete_longitude)
  ) role(role, region_name, channel_name, latitude, longitude)
  LEFT JOIN LATERAL (
    SELECT channel.channel_id
    FROM channel
    JOIN channel parent ON parent.channel_id = channel.parent_channel_id
    WHERE channel.country_code = 'RU'
      AND channel.channel_type = 'G'
      AND channel.geo_type = 'CITY'
      AND channel.name = role.channel_name
      AND abs(channel.latitude - role.latitude) <= 0.000001
      AND abs(channel.longitude - role.longitude) <= 0.000001
      AND parent.name = role.region_name
  ) c ON true
  GROUP BY s.merge_no, role.role
), resolved AS (
  SELECT merge_no,
         max(channel_ids) FILTER (WHERE role = 'keep') keep_ids,
         max(channel_ids) FILTER (WHERE role = 'delete') delete_ids
  FROM matches
  GROUP BY merge_no
), plan AS (
  SELECT merge_no, keep_ids[1] keep_channel_id,
         delete_ids[1] delete_channel_id
  FROM resolved
  WHERE coalesce(cardinality(keep_ids), 0) = 1
    AND coalesce(cardinality(delete_ids), 0) = 1
), included AS (
      SELECT t.ccg_id, coalesce(p.keep_channel_id, t.geo_channel_id) channel_id
      FROM ccggeochannel t
      LEFT JOIN plan p ON p.delete_channel_id = t.geo_channel_id
    ), excluded AS (
      SELECT t.ccg_id, coalesce(p.keep_channel_id, t.geo_channel_id) channel_id
      FROM ccggeochannelexcluded t
      LEFT JOIN plan p ON p.delete_channel_id = t.geo_channel_id
    )
    SELECT 1
    FROM included i
    JOIN excluded e USING (ccg_id, channel_id)
    WHERE i.channel_id IN (SELECT keep_channel_id FROM plan))
  THEN
    RAISE EXCEPTION 'merge creates conflicting included/excluded CCG targeting';
  END IF;

  IF EXISTS (
    WITH spec AS (
  SELECT *
  FROM jsonb_to_recordset(
    current_setting('foros.geo_channel_merge_plan')::jsonb)
  AS value(
    merge_no integer,
    keep_region text,
    keep_name text,
    keep_latitude numeric,
    keep_longitude numeric,
    delete_region text,
    delete_name text,
    delete_latitude numeric,
    delete_longitude numeric)
), matches AS (
  SELECT s.merge_no, role.role,
         array_agg(c.channel_id ORDER BY c.channel_id)
           FILTER (WHERE c.channel_id IS NOT NULL) AS channel_ids
  FROM spec s
  CROSS JOIN LATERAL (VALUES
    ('keep', s.keep_region, s.keep_name,
      s.keep_latitude, s.keep_longitude),
    ('delete', s.delete_region, s.delete_name,
      s.delete_latitude, s.delete_longitude)
  ) role(role, region_name, channel_name, latitude, longitude)
  LEFT JOIN LATERAL (
    SELECT channel.channel_id
    FROM channel
    JOIN channel parent ON parent.channel_id = channel.parent_channel_id
    WHERE channel.country_code = 'RU'
      AND channel.channel_type = 'G'
      AND channel.geo_type = 'CITY'
      AND channel.name = role.channel_name
      AND abs(channel.latitude - role.latitude) <= 0.000001
      AND abs(channel.longitude - role.longitude) <= 0.000001
      AND parent.name = role.region_name
  ) c ON true
  GROUP BY s.merge_no, role.role
), resolved AS (
  SELECT merge_no,
         max(channel_ids) FILTER (WHERE role = 'keep') keep_ids,
         max(channel_ids) FILTER (WHERE role = 'delete') delete_ids
  FROM matches
  GROUP BY merge_no
), plan AS (
  SELECT merge_no, keep_ids[1] keep_channel_id,
         delete_ids[1] delete_channel_id
  FROM resolved
  WHERE coalesce(cardinality(keep_ids), 0) = 1
    AND coalesce(cardinality(delete_ids), 0) = 1
), included AS (
      SELECT t.flight_id, coalesce(p.keep_channel_id, t.geo_channel_id) channel_id
      FROM flightgeochannel t
      LEFT JOIN plan p ON p.delete_channel_id = t.geo_channel_id
    ), excluded AS (
      SELECT t.flight_id, coalesce(p.keep_channel_id, t.channel_id) channel_id
      FROM flightgeochannelexcluded t
      LEFT JOIN plan p ON p.delete_channel_id = t.channel_id
    )
    SELECT 1
    FROM included i
    JOIN excluded e USING (flight_id, channel_id)
    WHERE i.channel_id IN (SELECT keep_channel_id FROM plan))
  THEN
    RAISE EXCEPTION 'merge creates conflicting included/excluded flight targeting';
  END IF;
END
$validation$;

WITH spec AS (
  SELECT *
  FROM jsonb_to_recordset(
    current_setting('foros.geo_channel_merge_plan')::jsonb)
  AS value(
    merge_no integer,
    keep_region text,
    keep_name text,
    keep_latitude numeric,
    keep_longitude numeric,
    delete_region text,
    delete_name text,
    delete_latitude numeric,
    delete_longitude numeric)
), matches AS (
  SELECT s.merge_no, role.role,
         array_agg(c.channel_id ORDER BY c.channel_id)
           FILTER (WHERE c.channel_id IS NOT NULL) AS channel_ids
  FROM spec s
  CROSS JOIN LATERAL (VALUES
    ('keep', s.keep_region, s.keep_name,
      s.keep_latitude, s.keep_longitude),
    ('delete', s.delete_region, s.delete_name,
      s.delete_latitude, s.delete_longitude)
  ) role(role, region_name, channel_name, latitude, longitude)
  LEFT JOIN LATERAL (
    SELECT channel.channel_id
    FROM channel
    JOIN channel parent ON parent.channel_id = channel.parent_channel_id
    WHERE channel.country_code = 'RU'
      AND channel.channel_type = 'G'
      AND channel.geo_type = 'CITY'
      AND channel.name = role.channel_name
      AND abs(channel.latitude - role.latitude) <= 0.000001
      AND abs(channel.longitude - role.longitude) <= 0.000001
      AND parent.name = role.region_name
  ) c ON true
  GROUP BY s.merge_no, role.role
), resolved AS (
  SELECT merge_no,
         max(channel_ids) FILTER (WHERE role = 'keep') keep_ids,
         max(channel_ids) FILTER (WHERE role = 'delete') delete_ids
  FROM matches
  GROUP BY merge_no
), plan AS (
  SELECT merge_no, keep_ids[1] keep_channel_id,
         delete_ids[1] delete_channel_id
  FROM resolved
  WHERE coalesce(cardinality(keep_ids), 0) = 1
    AND coalesce(cardinality(delete_ids), 0) = 1
), ids AS (
  SELECT keep_channel_id channel_id FROM plan
  UNION
  SELECT delete_channel_id FROM plan
)
SELECT c.channel_id
FROM channel c
JOIN ids USING (channel_id)
FOR UPDATE;

WITH spec AS (
  SELECT *
  FROM jsonb_to_recordset(
    current_setting('foros.geo_channel_merge_plan')::jsonb)
  AS value(
    merge_no integer,
    keep_region text,
    keep_name text,
    keep_latitude numeric,
    keep_longitude numeric,
    delete_region text,
    delete_name text,
    delete_latitude numeric,
    delete_longitude numeric)
), matches AS (
  SELECT s.merge_no, role.role,
         array_agg(c.channel_id ORDER BY c.channel_id)
           FILTER (WHERE c.channel_id IS NOT NULL) AS channel_ids
  FROM spec s
  CROSS JOIN LATERAL (VALUES
    ('keep', s.keep_region, s.keep_name,
      s.keep_latitude, s.keep_longitude),
    ('delete', s.delete_region, s.delete_name,
      s.delete_latitude, s.delete_longitude)
  ) role(role, region_name, channel_name, latitude, longitude)
  LEFT JOIN LATERAL (
    SELECT channel.channel_id
    FROM channel
    JOIN channel parent ON parent.channel_id = channel.parent_channel_id
    WHERE channel.country_code = 'RU'
      AND channel.channel_type = 'G'
      AND channel.geo_type = 'CITY'
      AND channel.name = role.channel_name
      AND abs(channel.latitude - role.latitude) <= 0.000001
      AND abs(channel.longitude - role.longitude) <= 0.000001
      AND parent.name = role.region_name
  ) c ON true
  GROUP BY s.merge_no, role.role
), resolved AS (
  SELECT merge_no,
         max(channel_ids) FILTER (WHERE role = 'keep') keep_ids,
         max(channel_ids) FILTER (WHERE role = 'delete') delete_ids
  FROM matches
  GROUP BY merge_no
), plan AS (
  SELECT merge_no, keep_ids[1] keep_channel_id,
         delete_ids[1] delete_channel_id
  FROM resolved
  WHERE coalesce(cardinality(keep_ids), 0) = 1
    AND coalesce(cardinality(delete_ids), 0) = 1
), affected AS (
  SELECT t.ccg_id
  FROM ccggeochannel t
  JOIN plan p ON p.delete_channel_id = t.geo_channel_id
  UNION ALL
  SELECT t.ccg_id
  FROM ccggeochannelexcluded t
  JOIN plan p ON p.delete_channel_id = t.geo_channel_id
)
UPDATE campaigncreativegroup ccg
SET version = clock_timestamp()
WHERE ccg.ccg_id IN (SELECT ccg_id FROM affected);

WITH spec AS (
  SELECT *
  FROM jsonb_to_recordset(
    current_setting('foros.geo_channel_merge_plan')::jsonb)
  AS value(
    merge_no integer,
    keep_region text,
    keep_name text,
    keep_latitude numeric,
    keep_longitude numeric,
    delete_region text,
    delete_name text,
    delete_latitude numeric,
    delete_longitude numeric)
), matches AS (
  SELECT s.merge_no, role.role,
         array_agg(c.channel_id ORDER BY c.channel_id)
           FILTER (WHERE c.channel_id IS NOT NULL) AS channel_ids
  FROM spec s
  CROSS JOIN LATERAL (VALUES
    ('keep', s.keep_region, s.keep_name,
      s.keep_latitude, s.keep_longitude),
    ('delete', s.delete_region, s.delete_name,
      s.delete_latitude, s.delete_longitude)
  ) role(role, region_name, channel_name, latitude, longitude)
  LEFT JOIN LATERAL (
    SELECT channel.channel_id
    FROM channel
    JOIN channel parent ON parent.channel_id = channel.parent_channel_id
    WHERE channel.country_code = 'RU'
      AND channel.channel_type = 'G'
      AND channel.geo_type = 'CITY'
      AND channel.name = role.channel_name
      AND abs(channel.latitude - role.latitude) <= 0.000001
      AND abs(channel.longitude - role.longitude) <= 0.000001
      AND parent.name = role.region_name
  ) c ON true
  GROUP BY s.merge_no, role.role
), resolved AS (
  SELECT merge_no,
         max(channel_ids) FILTER (WHERE role = 'keep') keep_ids,
         max(channel_ids) FILTER (WHERE role = 'delete') delete_ids
  FROM matches
  GROUP BY merge_no
), plan AS (
  SELECT merge_no, keep_ids[1] keep_channel_id,
         delete_ids[1] delete_channel_id
  FROM resolved
  WHERE coalesce(cardinality(keep_ids), 0) = 1
    AND coalesce(cardinality(delete_ids), 0) = 1
), affected AS (
  SELECT t.flight_id
  FROM flightgeochannel t
  JOIN plan p ON p.delete_channel_id = t.geo_channel_id
  UNION ALL
  SELECT t.flight_id
  FROM flightgeochannelexcluded t
  JOIN plan p ON p.delete_channel_id = t.channel_id
)
UPDATE flight f
SET version = clock_timestamp()
WHERE f.flight_id IN (SELECT flight_id FROM affected);

WITH spec AS (
  SELECT *
  FROM jsonb_to_recordset(
    current_setting('foros.geo_channel_merge_plan')::jsonb)
  AS value(
    merge_no integer,
    keep_region text,
    keep_name text,
    keep_latitude numeric,
    keep_longitude numeric,
    delete_region text,
    delete_name text,
    delete_latitude numeric,
    delete_longitude numeric)
), matches AS (
  SELECT s.merge_no, role.role,
         array_agg(c.channel_id ORDER BY c.channel_id)
           FILTER (WHERE c.channel_id IS NOT NULL) AS channel_ids
  FROM spec s
  CROSS JOIN LATERAL (VALUES
    ('keep', s.keep_region, s.keep_name,
      s.keep_latitude, s.keep_longitude),
    ('delete', s.delete_region, s.delete_name,
      s.delete_latitude, s.delete_longitude)
  ) role(role, region_name, channel_name, latitude, longitude)
  LEFT JOIN LATERAL (
    SELECT channel.channel_id
    FROM channel
    JOIN channel parent ON parent.channel_id = channel.parent_channel_id
    WHERE channel.country_code = 'RU'
      AND channel.channel_type = 'G'
      AND channel.geo_type = 'CITY'
      AND channel.name = role.channel_name
      AND abs(channel.latitude - role.latitude) <= 0.000001
      AND abs(channel.longitude - role.longitude) <= 0.000001
      AND parent.name = role.region_name
  ) c ON true
  GROUP BY s.merge_no, role.role
), resolved AS (
  SELECT merge_no,
         max(channel_ids) FILTER (WHERE role = 'keep') keep_ids,
         max(channel_ids) FILTER (WHERE role = 'delete') delete_ids
  FROM matches
  GROUP BY merge_no
), plan AS (
  SELECT merge_no, keep_ids[1] keep_channel_id,
         delete_ids[1] delete_channel_id
  FROM resolved
  WHERE coalesce(cardinality(keep_ids), 0) = 1
    AND coalesce(cardinality(delete_ids), 0) = 1
), aliases AS (
  SELECT p.keep_channel_id, 0 source_rank, 0::bigint source_id,
         alias.ordinality, btrim(alias.value) AS alias_value
  FROM (SELECT DISTINCT keep_channel_id FROM plan) p
  JOIN channel c ON c.channel_id = p.keep_channel_id
  CROSS JOIN LATERAL regexp_split_to_table(
    concat_ws(E'\n', c.name, c.city_list), E'[\r\n]+')
    WITH ORDINALITY alias(value, ordinality)
  UNION ALL
  SELECT p.keep_channel_id, 1, p.delete_channel_id,
         alias.ordinality, btrim(alias.value)
  FROM plan p
  JOIN channel c ON c.channel_id = p.delete_channel_id
  CROSS JOIN LATERAL regexp_split_to_table(
    concat_ws(E'\n', c.name, c.city_list), E'[\r\n]+')
    WITH ORDINALITY alias(value, ordinality)
), deduplicated AS (
  SELECT *, row_number() OVER (
    PARTITION BY keep_channel_id, lower(alias_value)
    ORDER BY source_rank, source_id, ordinality) AS duplicate_no
  FROM aliases
  WHERE alias_value <> ''
), merged AS (
  SELECT keep_channel_id,
       string_agg(alias_value, E'\n'
         ORDER BY source_rank, source_id, ordinality) AS city_list
  FROM deduplicated
  WHERE duplicate_no = 1
  GROUP BY keep_channel_id
)
UPDATE channel keep
SET city_list = merged.city_list,
    version = clock_timestamp()
FROM merged
WHERE keep.channel_id = merged.keep_channel_id;

WITH spec AS (
  SELECT *
  FROM jsonb_to_recordset(
    current_setting('foros.geo_channel_merge_plan')::jsonb)
  AS value(
    merge_no integer,
    keep_region text,
    keep_name text,
    keep_latitude numeric,
    keep_longitude numeric,
    delete_region text,
    delete_name text,
    delete_latitude numeric,
    delete_longitude numeric)
), matches AS (
  SELECT s.merge_no, role.role,
         array_agg(c.channel_id ORDER BY c.channel_id)
           FILTER (WHERE c.channel_id IS NOT NULL) AS channel_ids
  FROM spec s
  CROSS JOIN LATERAL (VALUES
    ('keep', s.keep_region, s.keep_name,
      s.keep_latitude, s.keep_longitude),
    ('delete', s.delete_region, s.delete_name,
      s.delete_latitude, s.delete_longitude)
  ) role(role, region_name, channel_name, latitude, longitude)
  LEFT JOIN LATERAL (
    SELECT channel.channel_id
    FROM channel
    JOIN channel parent ON parent.channel_id = channel.parent_channel_id
    WHERE channel.country_code = 'RU'
      AND channel.channel_type = 'G'
      AND channel.geo_type = 'CITY'
      AND channel.name = role.channel_name
      AND abs(channel.latitude - role.latitude) <= 0.000001
      AND abs(channel.longitude - role.longitude) <= 0.000001
      AND parent.name = role.region_name
  ) c ON true
  GROUP BY s.merge_no, role.role
), resolved AS (
  SELECT merge_no,
         max(channel_ids) FILTER (WHERE role = 'keep') keep_ids,
         max(channel_ids) FILTER (WHERE role = 'delete') delete_ids
  FROM matches
  GROUP BY merge_no
), plan AS (
  SELECT merge_no, keep_ids[1] keep_channel_id,
         delete_ids[1] delete_channel_id
  FROM resolved
  WHERE coalesce(cardinality(keep_ids), 0) = 1
    AND coalesce(cardinality(delete_ids), 0) = 1
)
INSERT INTO ccggeochannel(geo_channel_id, ccg_id, last_updated)
SELECT DISTINCT p.keep_channel_id, t.ccg_id, clock_timestamp()
FROM ccggeochannel t
JOIN plan p ON p.delete_channel_id = t.geo_channel_id
WHERE NOT EXISTS (
  SELECT 1
  FROM ccggeochannel existing
  WHERE existing.ccg_id = t.ccg_id
    AND existing.geo_channel_id = p.keep_channel_id);

WITH spec AS (
  SELECT *
  FROM jsonb_to_recordset(
    current_setting('foros.geo_channel_merge_plan')::jsonb)
  AS value(
    merge_no integer,
    keep_region text,
    keep_name text,
    keep_latitude numeric,
    keep_longitude numeric,
    delete_region text,
    delete_name text,
    delete_latitude numeric,
    delete_longitude numeric)
), matches AS (
  SELECT s.merge_no, role.role,
         array_agg(c.channel_id ORDER BY c.channel_id)
           FILTER (WHERE c.channel_id IS NOT NULL) AS channel_ids
  FROM spec s
  CROSS JOIN LATERAL (VALUES
    ('keep', s.keep_region, s.keep_name,
      s.keep_latitude, s.keep_longitude),
    ('delete', s.delete_region, s.delete_name,
      s.delete_latitude, s.delete_longitude)
  ) role(role, region_name, channel_name, latitude, longitude)
  LEFT JOIN LATERAL (
    SELECT channel.channel_id
    FROM channel
    JOIN channel parent ON parent.channel_id = channel.parent_channel_id
    WHERE channel.country_code = 'RU'
      AND channel.channel_type = 'G'
      AND channel.geo_type = 'CITY'
      AND channel.name = role.channel_name
      AND abs(channel.latitude - role.latitude) <= 0.000001
      AND abs(channel.longitude - role.longitude) <= 0.000001
      AND parent.name = role.region_name
  ) c ON true
  GROUP BY s.merge_no, role.role
), resolved AS (
  SELECT merge_no,
         max(channel_ids) FILTER (WHERE role = 'keep') keep_ids,
         max(channel_ids) FILTER (WHERE role = 'delete') delete_ids
  FROM matches
  GROUP BY merge_no
), plan AS (
  SELECT merge_no, keep_ids[1] keep_channel_id,
         delete_ids[1] delete_channel_id
  FROM resolved
  WHERE coalesce(cardinality(keep_ids), 0) = 1
    AND coalesce(cardinality(delete_ids), 0) = 1
)
INSERT INTO ccggeochannelexcluded(geo_channel_id, ccg_id, last_updated)
SELECT DISTINCT p.keep_channel_id, t.ccg_id, clock_timestamp()
FROM ccggeochannelexcluded t
JOIN plan p ON p.delete_channel_id = t.geo_channel_id
WHERE NOT EXISTS (
  SELECT 1
  FROM ccggeochannelexcluded existing
  WHERE existing.ccg_id = t.ccg_id
    AND existing.geo_channel_id = p.keep_channel_id);

WITH spec AS (
  SELECT *
  FROM jsonb_to_recordset(
    current_setting('foros.geo_channel_merge_plan')::jsonb)
  AS value(
    merge_no integer,
    keep_region text,
    keep_name text,
    keep_latitude numeric,
    keep_longitude numeric,
    delete_region text,
    delete_name text,
    delete_latitude numeric,
    delete_longitude numeric)
), matches AS (
  SELECT s.merge_no, role.role,
         array_agg(c.channel_id ORDER BY c.channel_id)
           FILTER (WHERE c.channel_id IS NOT NULL) AS channel_ids
  FROM spec s
  CROSS JOIN LATERAL (VALUES
    ('keep', s.keep_region, s.keep_name,
      s.keep_latitude, s.keep_longitude),
    ('delete', s.delete_region, s.delete_name,
      s.delete_latitude, s.delete_longitude)
  ) role(role, region_name, channel_name, latitude, longitude)
  LEFT JOIN LATERAL (
    SELECT channel.channel_id
    FROM channel
    JOIN channel parent ON parent.channel_id = channel.parent_channel_id
    WHERE channel.country_code = 'RU'
      AND channel.channel_type = 'G'
      AND channel.geo_type = 'CITY'
      AND channel.name = role.channel_name
      AND abs(channel.latitude - role.latitude) <= 0.000001
      AND abs(channel.longitude - role.longitude) <= 0.000001
      AND parent.name = role.region_name
  ) c ON true
  GROUP BY s.merge_no, role.role
), resolved AS (
  SELECT merge_no,
         max(channel_ids) FILTER (WHERE role = 'keep') keep_ids,
         max(channel_ids) FILTER (WHERE role = 'delete') delete_ids
  FROM matches
  GROUP BY merge_no
), plan AS (
  SELECT merge_no, keep_ids[1] keep_channel_id,
         delete_ids[1] delete_channel_id
  FROM resolved
  WHERE coalesce(cardinality(keep_ids), 0) = 1
    AND coalesce(cardinality(delete_ids), 0) = 1
)
INSERT INTO flightgeochannel(flight_id, geo_channel_id)
SELECT DISTINCT t.flight_id, p.keep_channel_id
FROM flightgeochannel t
JOIN plan p ON p.delete_channel_id = t.geo_channel_id
WHERE NOT EXISTS (
  SELECT 1
  FROM flightgeochannel existing
  WHERE existing.flight_id = t.flight_id
    AND existing.geo_channel_id = p.keep_channel_id);

WITH spec AS (
  SELECT *
  FROM jsonb_to_recordset(
    current_setting('foros.geo_channel_merge_plan')::jsonb)
  AS value(
    merge_no integer,
    keep_region text,
    keep_name text,
    keep_latitude numeric,
    keep_longitude numeric,
    delete_region text,
    delete_name text,
    delete_latitude numeric,
    delete_longitude numeric)
), matches AS (
  SELECT s.merge_no, role.role,
         array_agg(c.channel_id ORDER BY c.channel_id)
           FILTER (WHERE c.channel_id IS NOT NULL) AS channel_ids
  FROM spec s
  CROSS JOIN LATERAL (VALUES
    ('keep', s.keep_region, s.keep_name,
      s.keep_latitude, s.keep_longitude),
    ('delete', s.delete_region, s.delete_name,
      s.delete_latitude, s.delete_longitude)
  ) role(role, region_name, channel_name, latitude, longitude)
  LEFT JOIN LATERAL (
    SELECT channel.channel_id
    FROM channel
    JOIN channel parent ON parent.channel_id = channel.parent_channel_id
    WHERE channel.country_code = 'RU'
      AND channel.channel_type = 'G'
      AND channel.geo_type = 'CITY'
      AND channel.name = role.channel_name
      AND abs(channel.latitude - role.latitude) <= 0.000001
      AND abs(channel.longitude - role.longitude) <= 0.000001
      AND parent.name = role.region_name
  ) c ON true
  GROUP BY s.merge_no, role.role
), resolved AS (
  SELECT merge_no,
         max(channel_ids) FILTER (WHERE role = 'keep') keep_ids,
         max(channel_ids) FILTER (WHERE role = 'delete') delete_ids
  FROM matches
  GROUP BY merge_no
), plan AS (
  SELECT merge_no, keep_ids[1] keep_channel_id,
         delete_ids[1] delete_channel_id
  FROM resolved
  WHERE coalesce(cardinality(keep_ids), 0) = 1
    AND coalesce(cardinality(delete_ids), 0) = 1
)
INSERT INTO flightgeochannelexcluded(flight_id, channel_id)
SELECT DISTINCT t.flight_id, p.keep_channel_id
FROM flightgeochannelexcluded t
JOIN plan p ON p.delete_channel_id = t.channel_id
WHERE NOT EXISTS (
  SELECT 1
  FROM flightgeochannelexcluded existing
  WHERE existing.flight_id = t.flight_id
    AND existing.channel_id = p.keep_channel_id);

WITH spec AS (
  SELECT *
  FROM jsonb_to_recordset(
    current_setting('foros.geo_channel_merge_plan')::jsonb)
  AS value(
    merge_no integer,
    keep_region text,
    keep_name text,
    keep_latitude numeric,
    keep_longitude numeric,
    delete_region text,
    delete_name text,
    delete_latitude numeric,
    delete_longitude numeric)
), matches AS (
  SELECT s.merge_no, role.role,
         array_agg(c.channel_id ORDER BY c.channel_id)
           FILTER (WHERE c.channel_id IS NOT NULL) AS channel_ids
  FROM spec s
  CROSS JOIN LATERAL (VALUES
    ('keep', s.keep_region, s.keep_name,
      s.keep_latitude, s.keep_longitude),
    ('delete', s.delete_region, s.delete_name,
      s.delete_latitude, s.delete_longitude)
  ) role(role, region_name, channel_name, latitude, longitude)
  LEFT JOIN LATERAL (
    SELECT channel.channel_id
    FROM channel
    JOIN channel parent ON parent.channel_id = channel.parent_channel_id
    WHERE channel.country_code = 'RU'
      AND channel.channel_type = 'G'
      AND channel.geo_type = 'CITY'
      AND channel.name = role.channel_name
      AND abs(channel.latitude - role.latitude) <= 0.000001
      AND abs(channel.longitude - role.longitude) <= 0.000001
      AND parent.name = role.region_name
  ) c ON true
  GROUP BY s.merge_no, role.role
), resolved AS (
  SELECT merge_no,
         max(channel_ids) FILTER (WHERE role = 'keep') keep_ids,
         max(channel_ids) FILTER (WHERE role = 'delete') delete_ids
  FROM matches
  GROUP BY merge_no
), plan AS (
  SELECT merge_no, keep_ids[1] keep_channel_id,
         delete_ids[1] delete_channel_id
  FROM resolved
  WHERE coalesce(cardinality(keep_ids), 0) = 1
    AND coalesce(cardinality(delete_ids), 0) = 1
)
DELETE FROM ccggeochannel t
USING plan p
WHERE t.geo_channel_id = p.delete_channel_id;

WITH spec AS (
  SELECT *
  FROM jsonb_to_recordset(
    current_setting('foros.geo_channel_merge_plan')::jsonb)
  AS value(
    merge_no integer,
    keep_region text,
    keep_name text,
    keep_latitude numeric,
    keep_longitude numeric,
    delete_region text,
    delete_name text,
    delete_latitude numeric,
    delete_longitude numeric)
), matches AS (
  SELECT s.merge_no, role.role,
         array_agg(c.channel_id ORDER BY c.channel_id)
           FILTER (WHERE c.channel_id IS NOT NULL) AS channel_ids
  FROM spec s
  CROSS JOIN LATERAL (VALUES
    ('keep', s.keep_region, s.keep_name,
      s.keep_latitude, s.keep_longitude),
    ('delete', s.delete_region, s.delete_name,
      s.delete_latitude, s.delete_longitude)
  ) role(role, region_name, channel_name, latitude, longitude)
  LEFT JOIN LATERAL (
    SELECT channel.channel_id
    FROM channel
    JOIN channel parent ON parent.channel_id = channel.parent_channel_id
    WHERE channel.country_code = 'RU'
      AND channel.channel_type = 'G'
      AND channel.geo_type = 'CITY'
      AND channel.name = role.channel_name
      AND abs(channel.latitude - role.latitude) <= 0.000001
      AND abs(channel.longitude - role.longitude) <= 0.000001
      AND parent.name = role.region_name
  ) c ON true
  GROUP BY s.merge_no, role.role
), resolved AS (
  SELECT merge_no,
         max(channel_ids) FILTER (WHERE role = 'keep') keep_ids,
         max(channel_ids) FILTER (WHERE role = 'delete') delete_ids
  FROM matches
  GROUP BY merge_no
), plan AS (
  SELECT merge_no, keep_ids[1] keep_channel_id,
         delete_ids[1] delete_channel_id
  FROM resolved
  WHERE coalesce(cardinality(keep_ids), 0) = 1
    AND coalesce(cardinality(delete_ids), 0) = 1
)
DELETE FROM ccggeochannelexcluded t
USING plan p
WHERE t.geo_channel_id = p.delete_channel_id;

WITH spec AS (
  SELECT *
  FROM jsonb_to_recordset(
    current_setting('foros.geo_channel_merge_plan')::jsonb)
  AS value(
    merge_no integer,
    keep_region text,
    keep_name text,
    keep_latitude numeric,
    keep_longitude numeric,
    delete_region text,
    delete_name text,
    delete_latitude numeric,
    delete_longitude numeric)
), matches AS (
  SELECT s.merge_no, role.role,
         array_agg(c.channel_id ORDER BY c.channel_id)
           FILTER (WHERE c.channel_id IS NOT NULL) AS channel_ids
  FROM spec s
  CROSS JOIN LATERAL (VALUES
    ('keep', s.keep_region, s.keep_name,
      s.keep_latitude, s.keep_longitude),
    ('delete', s.delete_region, s.delete_name,
      s.delete_latitude, s.delete_longitude)
  ) role(role, region_name, channel_name, latitude, longitude)
  LEFT JOIN LATERAL (
    SELECT channel.channel_id
    FROM channel
    JOIN channel parent ON parent.channel_id = channel.parent_channel_id
    WHERE channel.country_code = 'RU'
      AND channel.channel_type = 'G'
      AND channel.geo_type = 'CITY'
      AND channel.name = role.channel_name
      AND abs(channel.latitude - role.latitude) <= 0.000001
      AND abs(channel.longitude - role.longitude) <= 0.000001
      AND parent.name = role.region_name
  ) c ON true
  GROUP BY s.merge_no, role.role
), resolved AS (
  SELECT merge_no,
         max(channel_ids) FILTER (WHERE role = 'keep') keep_ids,
         max(channel_ids) FILTER (WHERE role = 'delete') delete_ids
  FROM matches
  GROUP BY merge_no
), plan AS (
  SELECT merge_no, keep_ids[1] keep_channel_id,
         delete_ids[1] delete_channel_id
  FROM resolved
  WHERE coalesce(cardinality(keep_ids), 0) = 1
    AND coalesce(cardinality(delete_ids), 0) = 1
)
DELETE FROM flightgeochannel t
USING plan p
WHERE t.geo_channel_id = p.delete_channel_id;

WITH spec AS (
  SELECT *
  FROM jsonb_to_recordset(
    current_setting('foros.geo_channel_merge_plan')::jsonb)
  AS value(
    merge_no integer,
    keep_region text,
    keep_name text,
    keep_latitude numeric,
    keep_longitude numeric,
    delete_region text,
    delete_name text,
    delete_latitude numeric,
    delete_longitude numeric)
), matches AS (
  SELECT s.merge_no, role.role,
         array_agg(c.channel_id ORDER BY c.channel_id)
           FILTER (WHERE c.channel_id IS NOT NULL) AS channel_ids
  FROM spec s
  CROSS JOIN LATERAL (VALUES
    ('keep', s.keep_region, s.keep_name,
      s.keep_latitude, s.keep_longitude),
    ('delete', s.delete_region, s.delete_name,
      s.delete_latitude, s.delete_longitude)
  ) role(role, region_name, channel_name, latitude, longitude)
  LEFT JOIN LATERAL (
    SELECT channel.channel_id
    FROM channel
    JOIN channel parent ON parent.channel_id = channel.parent_channel_id
    WHERE channel.country_code = 'RU'
      AND channel.channel_type = 'G'
      AND channel.geo_type = 'CITY'
      AND channel.name = role.channel_name
      AND abs(channel.latitude - role.latitude) <= 0.000001
      AND abs(channel.longitude - role.longitude) <= 0.000001
      AND parent.name = role.region_name
  ) c ON true
  GROUP BY s.merge_no, role.role
), resolved AS (
  SELECT merge_no,
         max(channel_ids) FILTER (WHERE role = 'keep') keep_ids,
         max(channel_ids) FILTER (WHERE role = 'delete') delete_ids
  FROM matches
  GROUP BY merge_no
), plan AS (
  SELECT merge_no, keep_ids[1] keep_channel_id,
         delete_ids[1] delete_channel_id
  FROM resolved
  WHERE coalesce(cardinality(keep_ids), 0) = 1
    AND coalesce(cardinality(delete_ids), 0) = 1
)
DELETE FROM flightgeochannelexcluded t
USING plan p
WHERE t.channel_id = p.delete_channel_id;

WITH spec AS (
  SELECT *
  FROM jsonb_to_recordset(
    current_setting('foros.geo_channel_merge_plan')::jsonb)
  AS value(
    merge_no integer,
    keep_region text,
    keep_name text,
    keep_latitude numeric,
    keep_longitude numeric,
    delete_region text,
    delete_name text,
    delete_latitude numeric,
    delete_longitude numeric)
), matches AS (
  SELECT s.merge_no, role.role,
         array_agg(c.channel_id ORDER BY c.channel_id)
           FILTER (WHERE c.channel_id IS NOT NULL) AS channel_ids
  FROM spec s
  CROSS JOIN LATERAL (VALUES
    ('keep', s.keep_region, s.keep_name,
      s.keep_latitude, s.keep_longitude),
    ('delete', s.delete_region, s.delete_name,
      s.delete_latitude, s.delete_longitude)
  ) role(role, region_name, channel_name, latitude, longitude)
  LEFT JOIN LATERAL (
    SELECT channel.channel_id
    FROM channel
    JOIN channel parent ON parent.channel_id = channel.parent_channel_id
    WHERE channel.country_code = 'RU'
      AND channel.channel_type = 'G'
      AND channel.geo_type = 'CITY'
      AND channel.name = role.channel_name
      AND abs(channel.latitude - role.latitude) <= 0.000001
      AND abs(channel.longitude - role.longitude) <= 0.000001
      AND parent.name = role.region_name
  ) c ON true
  GROUP BY s.merge_no, role.role
), resolved AS (
  SELECT merge_no,
         max(channel_ids) FILTER (WHERE role = 'keep') keep_ids,
         max(channel_ids) FILTER (WHERE role = 'delete') delete_ids
  FROM matches
  GROUP BY merge_no
), plan AS (
  SELECT merge_no, keep_ids[1] keep_channel_id,
         delete_ids[1] delete_channel_id
  FROM resolved
  WHERE coalesce(cardinality(keep_ids), 0) = 1
    AND coalesce(cardinality(delete_ids), 0) = 1
)
UPDATE channel deleted
SET status = 'D',
    display_status_id = 5,
    status_change_date = clock_timestamp(),
    version = clock_timestamp()
FROM plan p
WHERE deleted.channel_id = p.delete_channel_id;

WITH spec AS (
  SELECT *
  FROM jsonb_to_recordset(
    current_setting('foros.geo_channel_merge_plan')::jsonb)
  AS value(
    merge_no integer,
    keep_region text,
    keep_name text,
    keep_latitude numeric,
    keep_longitude numeric,
    delete_region text,
    delete_name text,
    delete_latitude numeric,
    delete_longitude numeric)
), matches AS (
  SELECT s.merge_no, role.role,
         array_agg(c.channel_id ORDER BY c.channel_id)
           FILTER (WHERE c.channel_id IS NOT NULL) AS channel_ids
  FROM spec s
  CROSS JOIN LATERAL (VALUES
    ('keep', s.keep_region, s.keep_name,
      s.keep_latitude, s.keep_longitude),
    ('delete', s.delete_region, s.delete_name,
      s.delete_latitude, s.delete_longitude)
  ) role(role, region_name, channel_name, latitude, longitude)
  LEFT JOIN LATERAL (
    SELECT channel.channel_id
    FROM channel
    JOIN channel parent ON parent.channel_id = channel.parent_channel_id
    WHERE channel.country_code = 'RU'
      AND channel.channel_type = 'G'
      AND channel.geo_type = 'CITY'
      AND channel.name = role.channel_name
      AND abs(channel.latitude - role.latitude) <= 0.000001
      AND abs(channel.longitude - role.longitude) <= 0.000001
      AND parent.name = role.region_name
  ) c ON true
  GROUP BY s.merge_no, role.role
), resolved AS (
  SELECT merge_no,
         max(channel_ids) FILTER (WHERE role = 'keep') keep_ids,
         max(channel_ids) FILTER (WHERE role = 'delete') delete_ids
  FROM matches
  GROUP BY merge_no
), plan AS (
  SELECT merge_no, keep_ids[1] keep_channel_id,
         delete_ids[1] delete_channel_id
  FROM resolved
  WHERE coalesce(cardinality(keep_ids), 0) = 1
    AND coalesce(cardinality(delete_ids), 0) = 1
)
SELECT p.merge_no,
       keep.channel_id keep_channel_id, keep.name keep_name,
       deleted.channel_id deleted_channel_id, deleted.name deleted_name,
       deleted.status deleted_status
FROM plan p
JOIN channel keep ON keep.channel_id = p.keep_channel_id
JOIN channel deleted ON deleted.channel_id = p.delete_channel_id
ORDER BY p.merge_no;

COMMIT;
