#pragma once
#include <pgmspace.h>

const char webpage[] PROGMEM = R"=====(
<!doctype html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP Settings</title>
    <style>
        body {
            font-family: sans-serif;
            margin: 20px;
            background-color: #f4f4f4;
        }
        h1 {
            color: #333;
            text-align: center;
        }
        form {
            max-width: 420px;
            margin: 0 auto;
            padding: 20px;
            background-color: #fff;
            border-radius: 5px;
            box-shadow: 0 2px 5px rgba(0, 0, 0, 0.1);
        }
        label {
            display: block;
            margin-bottom: 5px;
            font-weight: bold;
            color: #555;
        }
        input[type="text"],
        input[type="number"] {
            width: 100%;
            padding: 10px;
            margin-bottom: 10px;
            border: 1px solid #ccc;
            border-radius: 3px;
            box-sizing: border-box;
        }
        button {
            background-color: #4CAF50;
            color: white;
            padding: 10px 20px;
            border: none;
            border-radius: 3px;
            cursor: pointer;
            width: 100%;
        }
        button:hover {
            background-color: #45a049;
        }
        .note {
            text-align: center;
            margin-top: 10px;
            color: #777;
            font-size: 14px;
        }
    </style>
</head>
<body>
    <h1>ESP NTP Clock Settings</h1>
    <form method="POST" action="/save">
        <label for="ntp">NTP server</label>
        <input type="text" id="ntp" name="ntp" value="{{NTP}}">

        <label for="city">City</label>
        <input type="text" id="city" name="city" value="{{CITY}}">

        <label for="key">OpenWeatherMap API key</label>
        <input type="text" id="key" name="key" value="{{KEY}}">

        <label for="tz">Offset TZ, sec</label>
        <input type="number" id="tz" name="tz" value="{{TZ}}">

        <button type="submit">💾 Save</button>
    </form>
    <div class="note">OTA update: /firmware</div>

    <div style="max-width: 420px; margin: 20px auto; padding: 15px; background: #fff;
        border-radius: 5px; box-shadow: 0 2px 5px rgba(0,0,0,0.1);">
        <div><b>Time:</b> <span id="time">--:--:--</span></div>
        <div><b>Date:</b> <span id="date">--.--.----</span></div>
        <div><b>Weather:</b> <span id="weather">...</span></div>
    </div>

    <script>
    async function updateState() {
        try {
            const r = await fetch("/state");
            if (!r.ok) return;
            const d = await r.json();
            document.getElementById("time").textContent = d.time;
            document.getElementById("date").textContent = d.date;
            document.getElementById("weather").textContent = d.weather;
        } catch (e) {}
    }
    updateState();
    setInterval(updateState, 1000);
    </script>
</body>
</html>

)=====";
