var lastsave=0;
var timer;

var CONFIG_CHANNEL="98429";
var CONFIG_APIKEY="JMXN5OQMY8ES50TY";

function temperature(newValue)
{
        document.getElementById("temp").innerHTML=newValue;
}

function effect(newValue)
{
        document.getElementById("effect").innerHTML=newValue;
}
function changekp(newValue)
{
        document.getElementById("kp").innerHTML=parseFloat(newValue).toFixed(1);
}
function changeki(newValue)
{
        document.getElementById("ki").innerHTML=parseFloat(newValue).toFixed(1);
}
function changekd(newValue)
{
        document.getElementById("kd").innerHTML=parseFloat(newValue).toFixed(1);
}

function changemode(newValue)
{
	if(newValue=="0"){
		m="Off";
//		document.getElementById("row_e").style.background = "red";
	}
	if(newValue=="1"){
		m="On";
	}
	if(newValue=="2"){
		m="PID";
	}
        document.getElementById("mode").innerHTML=newValue;
        document.getElementById("modestring").innerHTML=m;
}

function clear(){
   document.getElementById("log").innerHTML = "&nbsp;";
}

function load()
{
var url = "https://api.thingspeak.com/channels/" + CONFIG_CHANNEL + "/feeds/last?key=" + CONFIG_APIKEY;
var method = "GET";
var postData = "Some data";

var async = true;

var request = new XMLHttpRequest();

request.onload = function () {

   // Because of javascript's fabulous closure concept, the XMLHttpRequest "request"
   // object declared above is available in this function even though this function
   // executes long after the request is sent and long after this function is
   // instantiated. This fact is CRUCIAL to the workings of XHR in ordinary
   // applications.

   // You can get all kinds of information about the HTTP response.
   var status = request.status; // HTTP response status, e.g., 200 for "200 OK"
   var data = JSON.parse(request.responseText); // Returned data, e.g., an HTML document.
console.log(data);
   document.getElementById("mode").innerHTML = data.field1;
   document.getElementById("effect").innerHTML = data.field2;
   document.getElementById("temp").innerHTML = data.field3;
   document.getElementById("kp").innerHTML = data.field4;
   document.getElementById("ki").innerHTML = data.field5;
   document.getElementById("kd").innerHTML = data.field6;

   document.getElementById("range_m").value = data.field1;
   document.getElementById("range_e").value = data.field2;
   document.getElementById("range_t").value = data.field3;
   document.getElementById("range_p").value = data.field4;
   document.getElementById("range_i").value = data.field5;
   document.getElementById("range_d").value = data.field6;

   changemode(data.field1);

   document.getElementById("log").innerHTML = "load ok";

   window.setTimeout(clear, 1000);


}

request.open(method, url, async);

request.setRequestHeader("Content-Type", "application/json;charset=UTF-8");
//request.setRequestHeader("api-key", "JMXN5OQMY8ES50TY");
// Or... request.setRequestHeader("Content-Type", "text/plain;charset=UTF-8");
// Or... whatever

// Actually sends the request to the server.
request.send(postData);



}

function save(){


   var delay=0;

   if(lastsave>0){

      var d = new Date();
      var n = d.getTime();
      var diff = n-lastsave;

      if(diff<15000)
	delay=15000-diff;
      

   }

   console.log("waiting " + delay + " before saving");

   if(delay>0)
    document.getElementById("log").innerHTML = "saving in " + delay/1000  + " s";
   else
    document.getElementById("log").innerHTML = "saving..";

   if(timer!=null)
    clearTimeout(timer);

   timer=window.setTimeout(save2, delay);

}

function save2(){
 save3(document.getElementById("mode").innerHTML, 
      document.getElementById("effect").innerHTML, 
      document.getElementById("temp").innerHTML, 
      document.getElementById("kp").innerHTML, 
      document.getElementById("ki").innerHTML, 
      document.getElementById("kd").innerHTML);
}

function save3(m, e, t, p, i, d){
var url = "https://api.thingspeak.com/update?key=" + CONFIG_APIKEY;
var args = "&field1=" + m + "&field2=" + e + "&field3=" + t + "&field4=" + p + "&field5=" + i + "&field6=" + d;
var method = "GET";
var postData = "Some data";

var async = true;

var request = new XMLHttpRequest();

request.onload = function () {

   // Because of javascript's fabulous closure concept, the XMLHttpRequest "request"
   // object declared above is available in this function even though this function
   // executes long after the request is sent and long after this function is
   // instantiated. This fact is CRUCIAL to the workings of XHR in ordinary
   // applications.

   // You can get all kinds of information about the HTTP response.
   var status = request.status; // HTTP response status, e.g., 200 for "200 OK"
   var data = request.responseText; // Returned data, e.g., an HTML document.

   console.log(status);
   console.log(data);

   document.getElementById("log").innerHTML = "save ok";

   window.setTimeout(clear, 1000);

   var d = new Date();
   lastsave = d.getTime();

}

console.log(args);

request.open(method, url+args, async);

request.setRequestHeader("Content-Type", "application/json;charset=UTF-8");

// Actually sends the request to the server.
request.send(postData);

}
