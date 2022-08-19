var settings;

var settingsButton=document.getElementById("settings");
var osdSpan=document.getElementById("osd");
var line1Span=document.getElementById("line1");
var line2Span=document.getElementById("line2");
var osdDiv=document.getElementById("osddiv");
var modal = document.getElementById('myModal');
var closeModal = document.getElementsByClassName("close")[0];
var statusSpan=document.getElementById("status");
var saveButton = document.getElementById("save");
var rebootButton = document.getElementById("reboot");
var factoryresetButton = document.getElementById("factoryreset");

function checkKeycode(event) {
  // handling Internet Explorer stupidity with window.event
  // @see http://stackoverflow.com/a/3985882/517705
  var keyDownEvent = event || window.event,
      keycode = (keyDownEvent.which) ? keyDownEvent.which : keyDownEvent.keyCode;
  console.log("key code: " + keycode);
  switch(keycode) {
    case 8: numpad.delete(); break;
    case 13: numpad.select(); break;
    case 27: numpad.hide(); break;
    default:
      if((keycode>=0x60) && (keycode<=0x69)) {
        keycode-=0x30;
      }
      if((keycode>=0x30) && (keycode<=0x39)) {
        numpad.status.value = "";
        var current=numpad.display.value;
        keycode-=0x30;
        if ( current.length < numpad.max) {
          if (current=="0") {
            numpad.display.value = keycode;
          } else {
            numpad.display.value += keycode;
          }      
      }
    }
  }
}

function formToJSON()
{
  var form = document.getElementById("settingsForm");
  var formData = new FormData(form);
  var object = {};
  for(var pair of formData.entries()) {
    console.log("key: "+pair[0]+" value: "+pair[1]); 
  }
  object["wifi_opmode"]       = parseInt(formData.get("wifi_opmode"));
  object["wifi_ssid"]         = formData.get("wifi_ssid");
  object["wifi_secret"]       = formData.get("wifi_secret");
  object["wifi_hostname"]     = formData.get("wifi_hostname");
  object["wifi_powersave"]    = parseInt(formData.get("wifi_powersave"));
  object["wifi_ap_fallback"]  = parseInt(formData.get("wifi_ap_fallback"));
  
  object["tx_frequency"]      = parseInt(formData.get("tx_frequency"));
  object["sf"]                = parseInt(formData.get("sf"));
  object["bandwidth"]         = parseInt(formData.get("bandwidth"));
  object["tx_power"]          = parseInt(formData.get("tx_power"));
  object["syncword"]          = parseInt(formData.get("syncword"));
  object["tally_id"]          = parseInt(formData.get("tally_id"));
  object["num_pixels"]        = parseInt(formData.get("num_pixels"));
  object["tally_timeout"]     = parseInt(formData.get("tally_timeout"));
  object["display_timeout"]   = parseInt(formData.get("display_timeout"));
  object["led_max_brightness"]= parseInt(formData.get("led_max_brightness"));
  object["status_interval"]   = parseInt(formData.get("status_interval"));
  object["inactivity_timeout"]= parseInt(formData.get("inactivity_timeout"));
  object["mqtt_host"]         = formData.get("mqtt_host");
  object["atem_host"]         = formData.get("atem_host");
  object["tsl_port"]          = parseInt(formData.get("tsl_port"));
  object["command_interval"]  = parseInt(formData.get("command_interval"));
  
  return JSON.stringify(object);
}

function JSONToForm(form, json)
{
  settings = json;
  console.log(JSON.stringify(json));
  statusSpan.innerHTML="Version: "+json.version;
  switch(json.wifi_opmode) {
    case 0: document.getElementById("ap").checked=true; break;
    case 1: document.getElementById("sta").checked=true; break;
    case 2: document.getElementById("eth").checked=true; break;
  }
  document.getElementsByName("wifi_ssid")[0].value=json.wifi_ssid;
  document.getElementsByName("wifi_secret")[0].value=json.wifi_secret;
  document.getElementsByName("wifi_hostname")[0].value=json.wifi_hostname;
  document.getElementsByName("wifi_powersave")[0].value=json.wifi_powersave;
  switch(json.wifi_powersave) {
    case false: document.getElementById("wifi_powersave_off").checked=true; break;
    case true: document.getElementById("wifi_powersave_on").checked=true; break;
  }
  switch(json.wifi_ap_fallback) {
    case 0: document.getElementById("wifi_ap_fallback_off").checked=true; break;
    case 1: document.getElementById("wifi_ap_fallback_on").checked=true; break;
    case 2: document.getElementById("wifi_keep_sta").checked=true; break;
  }
  document.getElementsByName("tx_frequency")[0].value=json.tx_frequency;
  document.getElementsByName("sf")[0].value=json.sf;
  document.getElementsByName("bandwidth")[0].value=json.bandwidth;
  document.getElementsByName("tx_power")[0].value=json.tx_power;
  document.getElementsByName("syncword")[0].value=json.syncword;
  document.getElementsByName("tally_id")[0].value=json.tally_id;
  document.getElementsByName("num_pixels")[0].value=json.num_pixels;
  document.getElementsByName("tally_timeout")[0].value=json.tally_timeout;
  document.getElementsByName("display_timeout")[0].value=json.display_timeout;
  document.getElementsByName("led_max_brightness")[0].value=json.led_max_brightness;
  document.getElementsByName("mqtt_host")[0].value=json.mqtt_host;
  document.getElementsByName("status_interval")[0].value=json.status_interval;
  document.getElementsByName("command_interval")[0].value=json.command_interval;
  document.getElementsByName("atem_host")[0].value=json.atem_host;
  document.getElementsByName("tsl_port")[0].value=json.tsl_port;
  document.getElementsByName("inactivity_timeout")[0].value=json.inactivity_timeout;
}

function getSettings() {
  var xmlhttp = new XMLHttpRequest();
  var url = "settings.json";

  xmlhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
        settings = JSON.parse(this.responseText);
        JSONToForm("settingsForm", settings);
    }
  };
  xmlhttp.open("GET", url, true);
  xmlhttp.send();
}

function postSettings(json) {
  var xmlhttp = new XMLHttpRequest();
  var url = "settings.json";

  xmlhttp.open("POST", url, true);
  xmlhttp.setRequestHeader("Content-Type", "application/json");
  xmlhttp.onreadystatechange = function () {
      if (xmlhttp.readyState === 4 && xmlhttp.status === 200) {
        settings = JSON.parse(this.responseText); 
      }
      reboot();
    };
  
  xmlhttp.send(json);
}

function page(pager_num) {
  var xmlhttp = new XMLHttpRequest();
  if(settings["default_mode"] == 1) {
    pager_num=(pager_num&0x1fff)*8+700000;
  }
  var url = "page?pager_number="+pager_num;

  xmlhttp.onreadystatechange = function() {
    console.log("page: readyState:"+this.readyState+" status:"+this.status);
    if (this.readyState == 4) {
      if (this.status == 200) {
        numpad.status.value="OK";
      } else {
        numpad.status.value="FAIL";
        if(this.status < 400) {
          page(pager_num);
        }
      }
    }
  };
  xmlhttp.open("GET", url, true);
  xmlhttp.send();
}

function reboot() {
  var xmlhttp = new XMLHttpRequest();
  var url = "reboot";

  xmlhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
    }
  };
  xmlhttp.open("GET", url, true);
  xmlhttp.send();
}

function factoryreset() {
  var xmlhttp = new XMLHttpRequest();
  var url = "factoryreset";

  xmlhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
    }
  };
  xmlhttp.open("GET", url, true);
  xmlhttp.send();
}

settingsButton.onclick  = function()    {
  document.onkeydown = "";
  getSettings();
  modal.style.display = "block";
};
closeModal.onclick = function()         {
  document.onkeydown = checkKeycode;
  modal.style.display = "none"; 
};
saveButton.onclick = function()         {
  document.onkeydown = checkKeycode;
  modal.style.display = "none"; 
  jsonStr=formToJSON();
  console.log(jsonStr);
  postSettings(jsonStr);
};
rebootButton.onclick = function()       {
  modal.style.display = "none"; 
  reboot();
};
factoryresetButton.onclick = function () {
  modal.style.display = "none"; 
  var r=confirm("Do you really want to erase all settings?");
  if (r == true) {
    factoryreset();
  }
}
window.onclick = function(event) {
  if (event.target == modal) {
      document.onkeydown = checkKeycode;
      modal.style.display = "none";
  }
}

getSettings();

document.onkeydown = checkKeycode;
