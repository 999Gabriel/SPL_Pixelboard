# Google Sheets Setup for PixelBoard

To send temperature data from your ESP32 to a Google Sheet using your Google Account, follow these steps:

## 1. Create a Google Sheet
1. Open [Google Sheets](https://sheets.google.com).
2. Create a **New Spreadsheet**.
3. (Optional) Give it a name like "PixelBoard Weather Log".
4. The script will automatically use the first sheet (Sheet1).

## 2. Create the Google Apps Script
1. In your Google Sheet, go to **Extensions > Apps Script**.
2. A new tab will open with a code editor.
3. Delete all existing code.
4. Open the file `GOOGLE_APPS_SCRIPT.js` in this project and copy its entire content.
5. Paste the content into the Google Apps Script editor.
6. Click the **Save** icon (disk) and name the project (e.g., "PixelBoardLogger").

## 3. Deploy as a Web App (CRITICAL)
1. Click the **Deploy** button (top right) and select **New deployment**.
2. Click the "Select type" (gear icon) and choose **Web app**.
3. Fill in the following:
   - **Description:** ESP32 Logger
   - **Execute as:** Me (your email)
   - **Who has access:** Anyone
4. Click **Deploy**.
5. You may be asked to **Authorize access**. Click "Authorize access", choose your Google account, and click "Allow".
6. Once deployed, you will see a **Web App URL**. It looks like this:
   `https://script.google.com/macros/s/AKfycb.../exec`
7. **Copy this URL.**

## 4. Update ESP32 Code
1. Open `WeatherAPI_PixelBoard/src/main.cpp`.
2. Find the line:
   ```cpp
   const char* googleScriptUrl = "YOUR_GOOGLE_SCRIPT_URL_HERE";
   ```
3. Replace `"YOUR_GOOGLE_SCRIPT_URL_HERE"` with the URL you copied in the previous step.
4. Upload the code to your ESP32.

## 5. Verify
1. Open the Serial Monitor in VS Code/PlatformIO.
2. Watch for the message: `[GoogleSheets] Success! HTTP Code: 200`.
3. Check your Google Sheet; you should see new rows appearing every minute!

---

**Note:** If you ever change the Google Apps Script code, you must create a **New Deployment** (or update the existing one) to apply the changes.
