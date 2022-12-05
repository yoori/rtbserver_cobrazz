YAHOO.util.Event.onDOMReady(function () {
  var i, target;
  var gotoLinks = YAHOO.util.Dom.getElementsByClassName('gotolink', 'a');
  var targetLinks = [];
  // Build the list of target links first (so we can reset them)
  for (i = 0; i < gotoLinks.length; i++) {
    target = document.getElementById(gotoLinks[i].href.split('#')[1]);
    targetLinks[targetLinks.length] = target;
  }
  // targetLinks now contains all of the links that we link to
  for (i = 0; i < gotoLinks.length; i++) {
    YAHOO.util.Event.on(gotoLinks[i], 'click', function (e, t) {
      var j;
      var aname = this.href.split('#')[1];
      for (j = 0; j < t.length; j++) {
        YAHOO.util.Dom.replaceClass(t[j], 'activeLink', 'dormantLink');
      }
      YAHOO.util.Dom.replaceClass(aname, 'dormantLink', 'activeLink');
    }, targetLinks);
  }
});
