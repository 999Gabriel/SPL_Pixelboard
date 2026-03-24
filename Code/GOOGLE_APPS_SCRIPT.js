/**
 * Google Apps Script for ESP32 Weather Logging
 * 
 * Instructions:
 * 1. Create a new Google Sheet.
 * 2. Go to Extensions > Apps Script.
 * 3. Delete any existing code and paste this script.
 * 4. Click 'Deploy' > 'New Deployment'.
 * 5. Select 'Web App'.
 * 6. Set 'Execute as: Me'.
 * 7. Set 'Who has access: Anyone'.
 * 8. Deploy and copy the Web App URL.
 * 9. Paste the URL into your ESP32 code.
 */

function doGet(e) {
  return handleRequest(e);
}

function doPost(e) {
  return handleRequest(e);
}

function handleRequest(e) {
  // If the script is run manually from the editor, 'e' will be undefined.
  if (typeof e === 'undefined' || !e.parameter) {
    return ContentService.createTextOutput("Error: Script called without parameters. This is expected if you just clicked 'Run' in the editor. Test it by visiting the Web App URL in your browser with parameters, e.g., ?temp=20&hum=50&wind=5");
  }

  var ss = SpreadsheetApp.getActiveSpreadsheet();
  var sheet = ss.getSheets()[0];
  
  // Get data from parameters
  var temp = e.parameter.temp;
  var hum = e.parameter.hum;
  var wind = e.parameter.wind;
  var city = e.parameter.city || "Wattens";
  
  if (temp === undefined) {
    return ContentService.createTextOutput("Error: No 'temp' parameter received.");
  }

  // Append data to sheet: [Timestamp, City, Temperature, Humidity, Wind Speed]
  sheet.appendRow([new Date(), city, temp, hum, wind]);
  
  return ContentService.createTextOutput("Success: Data appended to sheet.");
}
