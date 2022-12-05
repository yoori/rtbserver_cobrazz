

cat PRImpression_*.csv | grep -E '^1,' | CsvFilter.pl --input='Input::Std(sep=>",")' --process='Process::Columns(field=>"11,12")' --process='Process::NotInFile(field=>"1",file=>"/home/jurij_kuznecov/PML2/click_uids")' --process='Output::Std' >/home/jurij_kuznecov/PML2/not_found_clicks

cat PRImpression_*.csv | grep -E '^1,' | CsvFilter.pl --input='Input::Std(sep=>",")' --process='Process::Columns(field=>"11,12")' --process='Process::InFile(field=>"1",file=>"/home/jurij_kuznecov/PML2/click_uids")' --process='Output::Std' >/home/jurij_kuznecov/PML2/found_clicks

cat /home/jurij_kuznecov/PML2/found_clicks | awk -F, '{print $2}' | sed -r 's|https?://||' | sed -r 's|^([^/]+)[/].*$|\1|' | awk '{print $1",1"}' >/home/jurij_kuznecov/PML2/f
cat /home/jurij_kuznecov/PML2/not_found_clicks | awk -F, '{print $2}' | sed -r 's|https?://||' | sed -r 's|^([^/]+)[/].*$|\1|' | awk '{print $1",0"}' >>/home/jurij_kuznecov/PML2/f

echo "domain,ya clicks/rtb clicks,rtb clicks,ya clicks" ; cat /home/jurij_kuznecov/PML2/f | sed 's/"//g' | sed -r 's/^www[.]//' | awk -F, 'NR>1{rtb_clicks[$1]++;clicks[$1]+=$2}END{for (a in rtb_clicks) print a","rtb_clicks[a]","clicks[a]}' | awk -F, '{if($2 > 0 && ($3 * 1.0 / $2) < 0.5){print $1","($3 * 1.0 / $2)","$2","$3}}'

