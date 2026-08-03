-- Fix OSVERSION Android detectors.
-- Broken form in DB (bytes): android N([.\\s_;]|$)
--   \\s became literal '\' + 's', NOT whitespace.
-- New form (no backslashes): android N([._;]|$)
--   '.'  -> Android 8.0.0 / 8.1.0
--   '_'  -> rare underscore forms
--   ';'  -> typical UA "Android 10; Pixel"
--   '$'  -> bare major at end
-- Does NOT match Android 18 / 80.
--
-- PG 9.4. After UPDATE: position(E'\\' IN match_regexp) must be 0.

BEGIN;

UPDATE public.platformdetector pd
SET match_regexp = 'android 8([._;]|$)', last_updated = now()
FROM public.platform p
WHERE pd.platform_id = p.platform_id AND p.type = 'OSVERSION' AND p.name = 'Android 8';

UPDATE public.platformdetector pd
SET match_regexp = 'android 9([._;]|$)', last_updated = now()
FROM public.platform p
WHERE pd.platform_id = p.platform_id AND p.type = 'OSVERSION' AND p.name = 'Android 9';

UPDATE public.platformdetector pd
SET match_regexp = 'android 10([._;]|$)', last_updated = now()
FROM public.platform p
WHERE pd.platform_id = p.platform_id AND p.type = 'OSVERSION' AND p.name = 'Android 10';

UPDATE public.platformdetector pd
SET match_regexp = 'android 11([._;]|$)', last_updated = now()
FROM public.platform p
WHERE pd.platform_id = p.platform_id AND p.type = 'OSVERSION' AND p.name = 'Android 11';

UPDATE public.platformdetector pd
SET match_regexp = 'android 12([._;]|$)', last_updated = now()
FROM public.platform p
WHERE pd.platform_id = p.platform_id AND p.type = 'OSVERSION' AND p.name = 'Android 12';

UPDATE public.platformdetector pd
SET match_regexp = 'android 13([._;]|$)', last_updated = now()
FROM public.platform p
WHERE pd.platform_id = p.platform_id AND p.type = 'OSVERSION' AND p.name = 'Android 13';

UPDATE public.platformdetector pd
SET match_regexp = 'android 14([._;]|$)', last_updated = now()
FROM public.platform p
WHERE pd.platform_id = p.platform_id AND p.type = 'OSVERSION' AND p.name = 'Android 14';

UPDATE public.platformdetector pd
SET match_regexp = 'android 15([._;]|$)', last_updated = now()
FROM public.platform p
WHERE pd.platform_id = p.platform_id AND p.type = 'OSVERSION' AND p.name = 'Android 15';

UPDATE public.platformdetector pd
SET match_regexp = 'android 16([._;]|$)', last_updated = now()
FROM public.platform p
WHERE pd.platform_id = p.platform_id AND p.type = 'OSVERSION' AND p.name = 'Android 16';

SELECT p.name, pd.match_regexp,
       length(pd.match_regexp) AS len,
       position(E'\\' IN pd.match_regexp) AS first_backslash_pos
FROM public.platformdetector pd
JOIN public.platform p ON p.platform_id = pd.platform_id
WHERE p.type = 'OSVERSION' AND p.name ~ '^Android [0-9]'
ORDER BY p.name;

COMMIT;
