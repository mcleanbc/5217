module.exports = [
  {
    "type": "heading",
    "defaultValue": "Set default working and resting times"
  },
  {
    "type": "slider",
    "messageKey": "workingTime",
    "defaultValue": 52,
    "label": "Working Time",
    "min": 1,
    "description": "Set the default working time (in minutes)."
  },
  {
    "type": "slider",
    "messageKey": "restingTime",
    "defaultValue": 17,
    "label": "Resting Time",
    "min": 1,
    "description": "Set the default resting time (in minutes)."
  },
  {
    "type": "toggle",
    "messageKey": "displayTime",
    "defaultValue": true,
    "label": "Display Current Time",
    "description": "Set whether to display the current time within the app."
  },
  {
  "type": "submit",
  "defaultValue": "Save"
  }
];