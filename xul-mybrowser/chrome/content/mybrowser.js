

/****************************************************************************************/
/* Globals
 */
var doctarget;
var toclick;
var state = 0;

playStateEnum = {
    idle : 0,
    optionLink : 1,
    finalLink : 2
};

searchStateEnum = {
    idle : 0,
    pricelink : 1,
    parsedetails : 2
};

var searchResults;
var searchDomResults;
var searchDetailsStartFlag = false;
var searchDetailsCount     = 0;
var searchState            = searchStateEnum.idle;
var searchSkipCount        = 0;
var searchIntervalId;
var searchStartFlag        = false;

var playItemId    = -1;
var playStartFlag = false;
var playState     = playStateEnum.idle;
var playIntervalId;
var playOption;

var thiscount = 0;


/* Movie details object */
var MovieInfo_t = {
    docid : '',
    title : '',
    href  : ''
};

/* Purchase option details */
var PurchaseOption_t = {
    type : 'none',
    quality : '',
    price : '0'
};

/****************************************************************************************/
function playParser()
{
    dump("in\n");
    switch(playState)
    {
        case playStateEnum.optionLink:
            {
                dump("in again\n");
                var options = doctarget.querySelectorAll('div.purchase-option-chooser');
                if(options && options.length > 0)
                {
                    for (var i=0, max=options.length; i < max; i++) 
                    {
                        dump("XXXXXXXXXXXXXXXXXXXXXXXXX found \n");
                        var content = options[0].getElementsByClassName('purchase-option');
                        for (var j=0, max2=content.length; j < max2; j++) 
                        {
                            var optionType = 'none';
                            var purchaseType = content[j].querySelectorAll('div.name');
                            if(content[j].style.display != 'none' && purchaseType && purchaseType.length>0)
                                for (var k=0, max3=purchaseType.length; k < max3; k++) 
                                {
                                    if(purchaseType[k].style.display != 'none')
                                    {
                                        optionType = purchaseType[k].innerHTML;
                                    }
                                }
                            /* Purchase options pricing and quality */
                            var prices = content[j].querySelectorAll('button.id-buy-single-option.buy-single-button.small.play-button.movies');
                            if(optionType == playOption.type && content[j].style.display != 'none' && prices && prices.length>0)
                                for (var k=0, max3=prices.length; k < max3; k++) 
                                {
                                    var quality = doctarget.getElementById('purchase-quality-buy-' + k);
                                    var price   = doctarget.getElementById('purchase-price-buy-' + k);
                                    if(quality && quality.innerHTML == playOption.quality && price && price.innerHTML == playOption.price)
                                    {
                                        var ev = doctarget.createEvent("MouseEvent");
                                        ev.initMouseEvent(
                                                "click",
                                                true /* bubble */, true /* cancelable */,
                                                window, null,
                                                0,0,0,0,  /* coordinates */
                                                false, false, false, false, /* modifier keys */
                                                0 /*left*/, null
                                                );
                                        prices[k].dispatchEvent(ev);
                                        playState = playStateEnum.finalLink;
                                    }
                                }
                        }
                    }
                }
            }
            break;

        case playStateEnum.finalLink:
            {
                var finalLink = doctarget.getElementById('loonie-purchase-ok-button');
                if(finalLink)
                    dump("C LE LIEN FINAAAAAAL\n");
            }
            break;
    }
}

/****************************************************************************************/
function resultParser()
{
    dump("toto task period " + searchState + "\n");
    switch(searchState)
    {
        case searchStateEnum.pricelink:
            {
                // Step 1: Click on price link
                var link = searchDomResults[searchDetailsCount].querySelectorAll('button.price.buy.id-track-click');
                if(link && link.length>0)
                {
                    var ev = doctarget.createEvent("MouseEvent");
                    ev.initMouseEvent(
                            "click",
                            true /* bubble */, true /* cancelable */,
                            window, null,
                            0,0,0,0,  /* coordinates */
                            false, false, false, false, /* modifier keys */
                            0 /*left*/, null
                            );
                    link[0].dispatchEvent(ev);
                    searchState = searchStateEnum.parsedetails;
                    // Purchase options
                    searchResults[searchDetailsCount].purchaseOptions = new Array();
                }
                else
                {
                    // Skip this item if a price button is not found
                    searchSkipCount++;
                    if(searchSkipCount > 1)
                    {
                        searchSkipCount = 0;
                        // Purchase options
                        searchResults[searchDetailsCount].purchaseOptions = new Array();
                        searchDetailsCount++;
                        if(searchDetailsCount >= searchDomResults.length)
                            searchState = searchStateEnum.idle;
                    }
                }
            }
            break;

        case searchStateEnum.parsedetails:
            {
                dump("step2\n");
                // Step 2 : Get the result details
                var options = doctarget.querySelectorAll('div.purchase-option-chooser');
                if(options && options.length > 0)
                {
                    for (var i=0, max=options.length; i < max; i++) {
                        dump("FFFFFF found " + searchDetailsCount + "/" + searchResults.length + "\n");
                        var content = options[0].getElementsByClassName('purchase-option');
                        for (var j=0, max2=content.length; j < max2; j++) {
                            var optionType = 'none';
                            //dump("option " + j + "\n");
                            //dump("style " + content[j].style.display + "\n");
                            //dump("code: " + content[j].innerHTML + "\n");
                            //var purchaseType = content[j].getElementById('purchase-name-buy-1');
                            var purchaseType = content[j].querySelectorAll('div.name');
                            if(content[j].style.display != 'none' && purchaseType && purchaseType.length>0)
                                for (var k=0, max3=purchaseType.length; k < max3; k++) 
                                {
                                    //dump("option " + purchaseType[k].innerHTML + "\n");
                                    //dump("style " + purchaseType[k].style.display + "\n");
                                    if(purchaseType[k].style.display != 'none')
                                    {
                                        optionType = purchaseType[k].innerHTML;
                                    }
                                }
                            /* Purchase options pricing and quality */
                            var prices = content[j].querySelectorAll('button.id-buy-single-option.buy-single-button.small.play-button.movies');
                            if(content[j].style.display != 'none' && prices && prices.length>0)
                                for (var k=0, max3=prices.length; k < max3; k++) 
                                {
                                    var option  = Object.create(PurchaseOption_t);
                                    option.type = optionType;

                                    var quality = doctarget.getElementById('purchase-quality-buy-' + k);
                                    if(quality)
                                        option.quality = quality.innerHTML;

                                    var price = doctarget.getElementById('purchase-price-buy-' + k);
                                    if(price)
                                        option.price = price.innerHTML;

                                    searchResults[searchDetailsCount].purchaseOptions.push(option);
                                }
                        }
                    }
                    var link = doctarget.querySelectorAll('button.close-dialog-button');
                    if(link && link.length>0)
                    {
                        var ev = doctarget.createEvent("MouseEvent");
                        ev.initMouseEvent(
                                "click",
                                true /* bubble */, true /* cancelable */,
                                window, null,
                                0,0,0,0,  /* coordinates */
                                false, false, false, false, /* modifier keys */
                                0 /*left*/, null
                                );
                        link[0].dispatchEvent(ev);
                    }
                    searchDetailsCount++;
                    if(searchDetailsCount < searchResults.length)
                        searchState = searchStateEnum.pricelink;
                    else
                    {
                        searchState = searchStateEnum.idle;
                        for (var i=0, max=searchResults.length; i < max; i++) {
                            dump("title: " + searchResults[i].title + "\n");
                            dump("href : " + searchResults[i].href + "\n");
                            dump("docid : " + searchResults[i].docid + "\n");
                            for (var j=0, max2=searchResults[i].purchaseOptions.length; j < max2; j++) {
                                dump("option: " + searchResults[i].purchaseOptions[j].type + "\n");
                                dump("price: " + searchResults[i].purchaseOptions[j].price + "\n");
                                dump("quality: " + searchResults[i].purchaseOptions[j].quality + "\n");
                            }
                        }
                        /* Stop the search polling */
                        clearInterval(searchIntervalId);
                        searchDetailsStartFlag = false;
                    }
                }
            }
            break;
    }
}

/****************************************************************************************/
// nsIWebProgressListener implementation to monitor activity in the browser.
function WebProgressListener() {
}
WebProgressListener.prototype = {
  _requestsStarted: 0,
  _requestsFinished: 0,

  // We need to advertize that we support weak references.  This is done simply
  // by saying that we QI to nsISupportsWeakReference.  XPConnect will take
  // care of actually implementing that interface on our behalf.
  QueryInterface: function(iid) {
    if (iid.equals(Components.interfaces.nsIWebProgressListener) ||
        iid.equals(Components.interfaces.nsISupportsWeakReference) ||
        iid.equals(Components.interfaces.nsISupports))
      return this;
    
    throw Components.results.NS_ERROR_NO_INTERFACE;
  },

  /****************************************************************************************/
  // This method is called to indicate state changes.
  onStateChange: function(webProgress, request, stateFlags, status) {
    const WPL = Components.interfaces.nsIWebProgressListener;

    var progress = document.getElementById("progress");

    if (stateFlags & WPL.STATE_IS_REQUEST) {
      if (stateFlags & WPL.STATE_START) {
        this._requestsStarted++;
        if(!this.thiscount)
        {
            this.thiscount = thiscount;
            thiscount++;
        }
        //dump("toto request start " + request.name + "\n");
      } else if (stateFlags & WPL.STATE_STOP) {
        //dump("toto request stop " + request.name + "\n");
        this._requestsFinished++;
      }
      if (this._requestsStarted > 1) {
        var value = (100 * this._requestsFinished) / this._requestsStarted;
        progress.setAttribute("mode", "determined");
        progress.setAttribute("value", value + "%");
        //if(request.name != "about:document-onload-blocker")
            //dump("request value: " + value + " name: " + request.name + "\n");
        //dump("request ref: " + request.QueryInterface(Components.interfaces.nslChannel).URI.spec + "\n");
        //request.QueryInterface(Components.interfaces.nslHttpChannel);
      }
    }

    if (stateFlags & WPL.STATE_IS_NETWORK) {
      var stop = document.getElementById("stop");
      if (stateFlags & WPL.STATE_START) {
        stop.setAttribute("disabled", false);
        progress.setAttribute("style", "");
        //dump("toto network start\n");
      } else if (stateFlags & WPL.STATE_STOP) {
        stop.setAttribute("disabled", true);
        progress.setAttribute("style", "display: none");
        this.onStatusChange(webProgress, request, 0, "Done");
        this._requestsStarted = this._requestsFinished = 0;
        dump("toto la c'est completement fini\n");

        // Parse the search results
        if(searchDomResults && !searchDetailsStartFlag && searchStartFlag)
        {
            searchIntervalId       = setInterval(resultParser, 500);
            searchDetailsStartFlag = true;
            searchStartFlag        = false;
            searchState            = searchStateEnum.pricelink;
        }

        // Start to play movie
        if(playStartFlag && doctarget && toclick)
        {
            var ev = doctarget.createEvent("MouseEvent");
            ev.initMouseEvent(
                    "click",
                    true /* bubble */, true /* cancelable */,
                    window, null,
                    0,0,0,0,  /* coordinates */
                    false, false, false, false, /* modifier keys */
                    0 /*left*/, null
                    );
            toclick.dispatchEvent(ev);
            // Start the play clik task
            playIntervalId = setInterval(playParser, 500);
            playStartFlag  = false;
            playState      = playStateEnum.optionLink;
            // Reset the click target 
            toclick   = null;
        }

        if(false && doctarget && toclick) {
            //for (var j=0, max2=toclick.length; j < max2; j++) {
            ////elements[i].click();
            ////eventFire(elements[i], 'click');
            //var Y = elements[i].offsetTop + 10;
            //var X = elements[i].offsetLeft + 10;
            ////var tamere = doc.elementFromPoint(X, Y);
            //dump("element id: " + elements[i].className + " " + X + "," + Y +"\n");
            //var viewportOffset = elements[i].getBoundingClientRect();
            //var top = viewportOffset.top + 10;
            //var left = viewportOffset.left + 10;
            //dump("element pos: " + left + "," + top +"\n");
            var ev = doctarget.createEvent("MouseEvent");
            ev.initMouseEvent(
                    "click",
                    true /* bubble */, true /* cancelable */,
                    window, null,
                    0,0,0,0,  /* coordinates */
                    false, false, false, false, /* modifier keys */
                    0 /*left*/, null
                    );
            //toclick[0].dispatchEvent(ev);
            toclick.dispatchEvent(ev);
            //doc.dispatchEvent(ev);
            doctarget = null;
            toclick   = null;
        }
      }
    }
  },

  /****************************************************************************************/
  // This method is called to indicate progress changes for the currently
  // loading page.
  onProgressChange: function(webProgress, request, curSelf, maxSelf,
                             curTotal, maxTotal) {
    if (this._requestsStarted == 1) {
      var progress = document.getElementById("progress");
      if (maxSelf == -1) {
        progress.setAttribute("mode", "undetermined");
      } else {
        progress.setAttribute("mode", "determined");
        progress.setAttribute("value", ((100 * curSelf) / maxSelf) + "%");
      }
    }
  },

  /****************************************************************************************/
  // This method is called to indicate a change to the current location.
  onLocationChange: function(webProgress, request, location) {
    var urlbar = document.getElementById("urlbar");
    urlbar.value = location.spec;

    var browser = document.getElementById("browser");
    var back = document.getElementById("back");
    var forward = document.getElementById("forward");

    back.setAttribute("disabled", !browser.canGoBack);
    forward.setAttribute("disabled", !browser.canGoForward);
    dump("toto on location change\n");
/*
    var all = webProgress.DOMWindow.document.getElementsByTagName("*");

    for (var i=0, max=all.length; i < max; i++) {
        dump("el: " + i + " " + all[i].id + "\n");
    }
*/
  },

  /****************************************************************************************/
  // This method is called to indicate a status changes for the currently
  // loading page.  The message is already formatted for display.
  onStatusChange: function(webProgress, request, status, message) {
    var status = document.getElementById("status");
    status.setAttribute("label", message);
  },

  /****************************************************************************************/
  // This method is called when the security state of the browser changes.
  onSecurityChange: function(webProgress, request, state) {
    const WPL = Components.interfaces.nsIWebProgressListener;

    var sec = document.getElementById("security");

    if (state & WPL.STATE_IS_INSECURE) {
      sec.setAttribute("style", "display: none");
    } else {
      var level = "unknown";
      if (state & WPL.STATE_IS_SECURE) {
        if (state & WPL.STATE_SECURE_HIGH)
          level = "high";
        else if (state & WPL.STATE_SECURE_MED)
          level = "medium";
        else if (state & WPL.STATE_SECURE_LOW)
          level = "low";
      } else if (state & WPL_STATE_IS_BROKEN) {
        level = "mixed";
      }
      sec.setAttribute("label", "Security: " + level);
      sec.setAttribute("style", "");
    }
  }
};

/****************************************************************************************/
var listener;

function go() {
  var urlbar = document.getElementById("urlbar");
  var browser = document.getElementById("browser");

  browser.loadURI(urlbar.value, null, null);
}

function back() {
  var browser = document.getElementById("browser");
  browser.stop();
  browser.goBack();
}

function forward() {
  var browser = document.getElementById("browser");
  browser.stop();
  browser.goForward();
}

function reload() {
  var browser = document.getElementById("browser");
  browser.reload();
}

function stop() {
  var browser = document.getElementById("browser");
  browser.stop();
}

function showConsole() {
  window.open("chrome://global/content/console.xul", "_blank",
    "chrome,extrachrome,menubar,resizable,scrollbars,status,toolbar");
}

var input;
var output;

function onload() {
  var urlbar = document.getElementById("urlbar");
  urlbar.value = "http://www.mozilla.org/";

  listener = new WebProgressListener();

  var browser = document.getElementById("browser");
  browser.addProgressListener(listener,
    Components.interfaces.nsIWebProgress.NOTIFY_ALL);

  dump("toto on load\n");
  go();

  var xhttp = new XMLHttpRequest();
  xhttp.open("POST", "http://localhost:8080/jsonrpc", false);
  xhttp.setRequestHeader("Content-type", "application/json");
  xhttp.setRequestHeader("Authorization", "authorization");

  var request = {};
  request.jsonrpc = "2.0";
  request.method  = "JSONRPC.Version";
  request.params  = {};
  request.id      = 1;

  dump(JSON.stringify(request) + "\n");

  xhttp.send(JSON.stringify(request));

  dump(xhttp.responseText + "\n");

  var reader = {
      onInputStreamReady : function(input) {
          var sin = Components.classes["@mozilla.org/scriptableinputstream;1"].createInstance(Components.interfaces.nsIScriptableInputStream);
          dump("Data in \n");
          try
          {
              sin.init(input);
              var readBytes = sin.available();
              var request   = '';
              request       = sin.read(readBytes);
              dump('Received: ' + request + "\n");
              //getUrl(request);
              output.write("yes", "yes".length);
              output.flush();

              var reqObj = JSON.parse(request);

              if('command' in reqObj)
                  dump('cmd: ' + reqObj.command + "\n");

              if('key2' in reqObj && 'skey2' in reqObj.key2)
                  dump('skey2: ' + reqObj.key2.skey2 + "\n");

              dump("contains toto is: " + ('toto' in reqObj) + "\n");

              switch(reqObj.command)
              {
                  case 'action_search':
                      action_search(reqObj.strSearch);
                      break;
              }

              //input.asyncWait(reader,0,0,null);
              var tm = Components.classes["@mozilla.org/thread-manager;1"].getService();
              input.asyncWait(reader,0,0,tm.mainThread);
          }
          catch(e)
          {
          }
      }
  }

  var listener = {
      onSocketAccepted: function(serverSocket, transport) {
          dump("Accepted connection on " + transport.host + ":" + transport.port + "\n");
          input  = transport.openInputStream(Components.interfaces.nsITransport.OPEN_BLOCKING, 0, 0);//.QueryInterface(Ci.nsIAsyncInputStream);
          output = transport.openOutputStream(Components.interfaces.nsITransport.OPEN_BLOCKING, 0, 0);

          //input.asyncWait(reader,0,0,null);
          var tm = Components.classes["@mozilla.org/thread-manager;1"].getService();
          input.asyncWait(reader,0,0,tm.mainThread);

/*
          finally
          {
              sin.close();
              input.close();
              output.close();
          }
*/
      }
  }

  var serverSocket = Components.classes["@mozilla.org/network/server-socket;1"].createInstance(Components.interfaces.nsIServerSocket);
  serverSocket.init(9090, true, 5);
  dump("Opened socket on " + serverSocket.port + "\n");
  serverSocket.asyncListen(listener);

/*
  var transportService =
      Components.classes["@mozilla.org/network/socket-transport-service;1"].getService(Components.interfaces.nsISocketTransportService);
  //alert(transportService);
  var transport =
      transportService.createTransport(null,0,"localhost",9009,null);
  //alert(transport);

  var stream =
      transport.openInputStream(Components.interfaces.nsITransport.OPEN_UNBUFFERED,null,null);
  //alert(stream);
  var instream =
      Components.classes["@mozilla.org/scriptableinputstream;1"].createInstance(Components.interfaces.nsIScriptableInputStream);
  //alert(instream);
  instream.init(stream);
  //alert(instream);

  var outstream =
      transport.openOutputStream(0,0,0);
      //Components.interfaces.nsITransport.OPEN_UNBUFFERED
*/

/*
  pmrpc.call( {
      destination : "publish",
      publicProcedureName : "Version",
      params : ["]
  } );
*/


  /* node.js init */
/*
  // Create an nsILocalFile for the Node executable
  var file = Components.classes["@mozilla.org/file/local;1"].createInstance(Components.interfaces.nsILocalFile);  
  file.initWithPath("/usr/bin/nodejs");

  // Check for Node executable
  if (!file.exists()) {
      // Node executable is missing
      dump("pas d'exe\n");
      return;
  }
  else 
      dump("exe OK\n");
  // Create an nsIProcess to manage the Node executable
  var process = Components.classes["@mozilla.org/process/util;1"].createInstance(Components.interfaces.nsIProcess);
  process.init(file);
}

function eventFire(el, etype){
    if (el.fireEvent) {
        el.fireEvent('on' + etype);
    } else {
        var evObj = document.createEvent('Events');
        evObj.initEvent(etype, true, false);
        el.dispatchEvent(evObj);
    }
*/
}

/****************************************************************************************/
function onCommand(aEvent) {
    dump("-- " + state + "\n");
    dump("target " + aEvent.target.className + "\n");
    dump("client " + aEvent.clientX + " " + aEvent.clientY + "\n");
    dump("offset " + aEvent.offsetX + " " + aEvent.offsetY + "\n");
    dump("page " + aEvent.pageX + " " + aEvent.pageY + "\n");
    dump("screen " + aEvent.screenX + " " + aEvent.screenY + "\n");

/*
    if(doctarget)
    {
        var elements = doctarget.getElementsByClassName('buy-button-container');
        //var elements = doctarget.getElementsByClassName('price buy id-track-click');
        if(elements && state == 1)
        {
            dump("parsing elements\n");
            state = 2;
            //dump("button found " + elements.length + "\n");
            for (var i=0, max=elements.length; i < max; i++) {
                //dump("button id: " + i + " " + elements[i].id + "\n");
                if (elements[i].hasAttributes()) {
                    for each ( var item in elements[i].attributes) {
                        //dump("prop: " + item.nodeName + ": " + item.nodeValue + "\n");
                        if(item.nodeName == "data-docid" && item.nodeValue == "movie-Tz1UYAn5fOA")
                        {
                            dump("id " + i + ": " + elements[i].className + "\n");
                            //elements[i].click();
                            //eventFire(elements[i], 'click');
                            var toclick = elements[i].getElementsByClassName('price buy id-track-click');
                            for (var j=0, max2=toclick.length; j < max2; j++) {
                                var Y = elements[i].offsetTop + 10;
                                var X = elements[i].offsetLeft + 10;
                                var viewportOffset = elements[i].getBoundingClientRect();
                                var top = viewportOffset.top + 10;
                                var left = viewportOffset.left + 10;
                                dump("elements pos: " + left + "," + top +"\n");
                                var ev = doctarget.createEvent("MouseEvent");
                                ev.initMouseEvent(
                                        "click",
                                        true, true,
                                        window, null,
                                        0,0,left, top,
                                        false, false, false, false,
                                        0, null
                                        );
                                //elements[i].dispatchEvent(ev);
                                toclick[j].dispatchEvent(ev);
                                //doc.dispatchEvent(ev);
                            }
                        }
                    }
                }
            }
            //aEvent.stopPropagation();
        }
    }
*/
}

/****************************************************************************************/
// Action create account
//

// Action sign in 
// Need: Credentials
// Return: Success state
// FIXME: Comment tester si on est deja signed in ou pas??
function action_log_in()
{
  // Load the account sign in page
  var browser = document.getElementById("browser");

  browser.loadURI("https://accounts.google.com/ServiceLogin?continue=https%3A%2F%2Fplay.google.com%2Fstore&service=googleplay&sacu=1&passive=1209600&acui=0#Email=pierro.buro%40gmail.com", null, null);
}

// Action item search 
// Need: item name
// Return: Search results
//         Item icons
//         Title
//         Rent / Buy
//         Pricing
//         HD / SD
function action_search(strSearch)
{
  // Load the search result
  var browser = document.getElementById("browser");

  searchStartFlag = true;
  //browser.loadURI("://play.google.com/store/search?q=ted2&c=movies", null, null);
  //browser.loadURI("://play.google.com/store/search?q=star wars&c=movies&docType=6", null, null);
  //browser.loadURI("://play.google.com/store/search?q=comment tuer son boss&c=movies&docType=6", null, null);
  browser.loadURI("://play.google.com/store/search?q=" + strSearch + "&c=movies&docType=6", null, null);
}

// Action Rent/Buy
// Need: Flag buy
//       Item identification
// Return: Sucess State
function action_play()
{
  var browser = document.getElementById("browser");

  playStartFlag = true;
  playItemId    = 18;
  playOption    = searchResults[18].purchaseOptions[1];
  browser.loadURI(searchResults[18].href, null, null);
}

// Action Control Player
// Need: subaction
// Return: Sucess state





/****************************************************************************************/
function onPageLoad(aEvent) {
    var doc = aEvent.originalTarget; // doc is document that triggered the event
    var win = doc.defaultView; // win is the window for the doc
    // test desired conditions and do something
    // if (doc.nodeName != "#document") return; // only documents
    // if (win != win.top) return; //only top window.
    // if (win.frameElement) return; // skip iframes/frames
    dump("page is loaded \n" + doc.location.href + "\n");

/* CHECK THE LOGGIN RESULT HERE ???
if( state == logging in && href == "play.google.com/store" )
state = logged_idle
*/

/*
    var all = doc.getElementsByTagName("*");
    for (var i=0, max=all.length; i < max; i++) {
        dump("el: " + i + " " + all[i].id + "\n");
    }
*/

    var flagnext = 0;

    var element = doc.getElementById("fundraiser-form");
    if(element)
    {
        dump("element found\n");
        element.style.display = "none";
    }
    var elements = doc.getElementsByClassName('buy-button-container');
    if(elements)
    {
        //dump("button found " + elements.length + "\n");
        for (var i=0, max=elements.length; i < max; i++) {
            //dump("button id: " + i + " " + elements[i].id + "\n");
            if (elements[i].hasAttributes()) {
                for each ( var item in elements[i].attributes) {
                    //dump("prop: " + item.nodeName + ": " + item.nodeValue + "\n");
                    if(item.nodeName == "data-docid" && item.nodeValue == "movie-Tz1UYAn5fOA")
                    {
                        flagnext = 1;
                        dump("XXXXX found you\n");
                        var tmp = elements[i].getElementsByClassName('price buy id-track-click');
                        dump("button id: " + i + " " + tmp[0].id + " class : " + tmp[0].className + "\n");
                        toclick = tmp[0];
                    }
                }
            }
        }
        if(flagnext == 1)
        {
            state = 1;
            doctarget = doc;
        }
    }
/*
    var signin = doc.getElementsByTagName('a');
    if(signin)
    {
        for (var i=0, max=signin.length; i < max; i++) {
            if( signin[i].href.indexOf("accounts.google.com") !=-1 )
            {
                dump("button id: " + i + " " + signin[i].id + " class : " + signin[i].className + "\n");
                dump("href: " + signin[i].href + "\n");
                toclick = signin[i];
                doctarget = doc;
            }
        }
    }
*/

    // Start play movie
    if(playStartFlag)
    {
        //var links = doc.querySelectorAll('button.price.buy.id-track-click');
        var links = doc.getElementsByClassName('buy-button-container');
        for (var i=0, max=links.length; i < max; i++) 
        {
            if (links[i].hasAttributes()) 
            {
                for each ( var item in links[i].attributes) 
                {
                    if(item.nodeName == "data-docid" && item.nodeValue == searchResults[playItemId].docid)
                    {
                        doctarget = doc;
                        //toclick   = links[i];
                        var buttons = links[i].querySelectorAll('button.price.buy.id-track-click');
                        toclick     = buttons[0];
                    }
                }
            }
        }
    }

    // Search results
    if(searchStartFlag)
    {
        var results = doc.getElementsByClassName('card no-rationale');
        if(results && results.length>0)
        {
            searchResults = new Array();
            for (var i=0, max=results.length; i < max; i++) {
                var movieInfo = Object.create(MovieInfo_t);
                // Movie id
                if (results[i].hasAttributes()) {
                    for each ( var item in results[i].attributes) {
                        if(item.nodeName == "data-docid")
                           movieInfo.docid = item.nodeValue;
                    }
                }
                // Movie title and url
                var link = results[i].querySelectorAll('a.title');
                if (link[0].hasAttributes()) {
                    for each ( var item in link[0].attributes) {
                        if(item.nodeName == "title") 
                            movieInfo.title = item.nodeValue;
                    }
                    // Movie url
                    movieInfo.href = link[0].href;

                }
                searchResults.push(movieInfo);
                dump("result: " + i + " " + results[i].id + " class : " + results[i].className + " title: " + movieInfo.title + "\n");
            }
            searchDomResults   = results;
            doctarget          = doc;
        }
    }

    // Account Sign in
    var signin = doc.getElementsByTagName('input');
    if(signin)
    {
        for (var i=0, max=signin.length; i < max; i++) {
            if( signin[i].id == 'signIn' )
            {
                dump("button id: " + i + " " + signin[i].id + " class : " + signin[i].className + "\n");
                toclick   = signin[i];
                doctarget = doc;
            }
            if( signin[i].id == 'Passwd' )
            {
                dump("button id: " + i + " " + signin[i].id + " class : " + signin[i].className + "\n");
                signin[i].value = "JkMt1976";
            }
        }
    }
}

addEventListener("load", onload, true);
addEventListener("DOMContentLoaded", onPageLoad, false);
addEventListener("click", onCommand, true);
