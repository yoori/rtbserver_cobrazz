\set ON_ERROR_STOP on
BEGIN;

-- The Perm village Sibir is at 58.251362, 56.241098.  The old point near
-- 57.8143, 51.3412 belongs to a different locality in Kirov region.
-- Sources checked on 2026-08-16:
-- https://www.komandirovka.ru/cities/sibirwck/
-- https://yandex.com/maps/geo/derevnya_sibir/53093222/
DO $validation$
BEGIN
  IF (
    SELECT count(*)
    FROM channel city
    JOIN channel region ON region.channel_id = city.parent_channel_id
    WHERE city.country_code = 'RU'
      AND city.channel_type = 'G'
      AND city.geo_type = 'CITY'
      AND city.name = 'Sibir1'
      AND region.name = 'Permskiy Kray'
      AND abs(city.latitude - 57.814300) <= 0.000001
      AND abs(city.longitude - 51.341200) <= 0.000001
  ) <> 1 THEN
    RAISE EXCEPTION 'Permskiy Kray/Sibir1 must resolve to one old coordinate';
  END IF;
END
$validation$;

UPDATE channel city
SET latitude = 58.251362,
    longitude = 56.241098,
    version = clock_timestamp()
FROM channel region
WHERE region.channel_id = city.parent_channel_id
  AND city.country_code = 'RU'
  AND city.channel_type = 'G'
  AND city.geo_type = 'CITY'
  AND city.name = 'Sibir1'
  AND region.name = 'Permskiy Kray'
  AND abs(city.latitude - 57.814300) <= 0.000001
  AND abs(city.longitude - 51.341200) <= 0.000001
RETURNING city.name, region.name, city.latitude, city.longitude;

COMMIT;
