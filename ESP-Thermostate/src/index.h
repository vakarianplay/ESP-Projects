#include <Arduino.h>

const char webpage[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<style type="text/css">
:root {
  --checkbox-slider-offset: 10000px;
  --checkbox-slider-size: 30px;
  --checkbox-slider-toggle: calc(var(--checkbox-slider-size) * 0.8);
  font-family: Helvetica;
}
  .checkbox-slider {
    font-size: 25pt;
  }

@supports (--a: 1) { 
  input[type="checkbox"].checkbox-slider,
  .checkbox-slider input[type="checkbox"]
  {
    position: relative;
    left: var(--checkbox-slider-offset);
    height: var(--checkbox-slider-size);
    width: var(--checkbox-slider-size);

    &::before, &::after {
      position: absolute;
      content: '';
      display: block;
      cursor: pointer;
      left: calc(var(--checkbox-slider-offset) * -1);
    }

    &::before {
      border-radius: var(--checkbox-slider-size);
      width: calc(var(--checkbox-slider-size) * 2);
      height: var(--checkbox-slider-size);
      background: #aaa;
      transition: background-color 200ms;
    }

    &:checked::before {
      background: #3f729b;
    }

    &::after {
      width: var(--checkbox-slider-toggle);
      height: var(--checkbox-slider-toggle);
      background: #fff;
      top: 50%;
      transform: translateY(-50%) translateX(13%);
      border-radius: var(--checkbox-slider-toggle);
      transition: box-shadow 100ms ease-in-out, transform 100ms ease-in-out;
    }

    &:checked::after {
      box-shadow: 0 2px 5px rgba(0, 0, 0, 0.4);
      transform: translateY(-50%) translateX(133%);
    }
  }
 
}
.button {
  background-color: #4CAF50; /* Green */
  border: none;
  color: white;
  padding: 15px 32px;
  text-align: center;
  text-decoration: none;
  display: inline-block;
  font-size: 16px;
}
</style>
</style>
<body>
<center>
<div>
<h1>ESP THERMOSTATE</h1>
  <p class="checkbox-slider">RELAY 1 <input type="checkbox" onclick="toggle1()" id="state1" unchecked>
  <p class="checkbox-slider">RELAY 2 <input type="checkbox" onclick="toggle2()" id="state2" unchecked>
</div>
 <br>
<div><h2>
  Temp(C): <span id="temp_val">0</span><br><br>
  Humidity(%): <span id="hum_val">NA</span>
</h2>
</div>
<script>

function toggle1() 
{
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      if (this.responseText == "0") {
        document.getElementById("state1").checked = false;
      // document.getElementById("state1").innerHTML = "RELAY1 OFF";
      } else 
        document.getElementById("state1").checked = true;
        // document.getElementById("state1").innerHTML = "RELAY1 ON";
    }
  };
  console.log('Toggle func 1');
  xhttp.open("GET", "toggleRelay1", true);
  xhttp.send();
}

function toggle2() 
{
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      if (this.responseText == "0") {
        document.getElementById("state2").checked = false;
      // document.getElementById("state2").innerHTML = "RELAY2 OFF";
      } else
        document.getElementById("state2").checked = true;
        // document.getElementById("state2").innerHTML = "RELAY2 ON";
    }
  };
  xhttp.open("GET", "toggleRelay2", true);
  xhttp.send();
}

function getRelay1() 
{
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      if (this.responseText == "0") {
        document.getElementById("state1").checked = false;
      // document.getElementById("state1").innerHTML = "RELAY1 OFF";
      } else 
        document.getElementById("state1").checked = true;
        // document.getElementById("state1").innerHTML = "RELAY1 ON";
    }
  };
  console.log('UPD func 1');
  xhttp.open("GET", "st1", true);
  xhttp.send();
}

function getRelay2() 
{
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      if (this.responseText == "0") {
        document.getElementById("state2").checked = false;
      // document.getElementById("state2").innerHTML = "RELAY2 OFF";
      } else
        document.getElementById("state2").checked = true;
        // document.getElementById("state2").innerHTML = "RELAY2 ON";
    }
  };
  xhttp.open("GET", "st2", true);
  xhttp.send();
}

function getData() {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("temp_val").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "readTemp", true);
  xhttp.send();
}

setInterval(function() 
{
  getData();
  getRelay1();
  getRelay2();
}, 2000); 

document.addEventListener("DOMContentLoaded", function() {
  getData();
  getRelay1();
  getRelay2();
});


</script>
</center>
</body>
</html>
)=====";