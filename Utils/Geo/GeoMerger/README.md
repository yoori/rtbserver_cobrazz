# GeoMerger

`GeoMerger` overlays mapped Geo observations on the existing IPv4 database.
It treats network ownership and routing data as aggregation boundaries, not as
location evidence.

The input Geo CSV must be grouped by `IP`. `Geo_aggregated.csv`, produced by
`sort -u`, has this property.

```bash
python3 Utils/Geo/BuildGeoIP.py \
  --geo ~/Geo_aggregated.csv \
  --mapping ~/geo_mapping_updated/city_mapping.csv \
  --existing /usr/share/GeoIP/ipv4.csv \
  --rir-database ripe.db.inetnum.gz \
  --bgp-prefixes prefix_origins.csv \
  --output ipv4.csv \
  --conflicts conflicts.csv \
  --summary summary.json
```

Defaults:

* observations start in `/25` cells, which are split at every hard boundary
  before support is counted;
* old database prefixes longer than `/24` are hard boundaries;
* the most specific containing RIR `inetnum` is a hard boundary;
* different BGP prefixes or origin sets are hard boundaries;
* siblings merge bottom-up only when both selected the same location;
* merging stops at `/23`;
* a cell needs two unique IPs and winner confidence of at least `0.75`.

RIR, BGP, and old-database data constrain aggregation only. They never supply
or override a location. A subnet is selected from Geo observations inside that
exact boundary-constrained cell, so observations on opposite sides of a
boundary cannot satisfy each other's support threshold.

RIR input is an RPSL database containing `inetnum:` objects. Plain text and
gzip files are accepted. BGP input is CSV or TSV with a `Prefix` or `Network`
column and an optional `OriginASN`, `Origin`, or `ASN` column.
