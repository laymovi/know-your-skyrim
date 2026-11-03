Scriptname KnowYourSkyrim Native Hidden
{Functions the plugin KnowYourSkyrim.dll gives to scripts}

; a setting by its id as the menu names it, "bEnabled:Cheats" is bEnabled in [Cheats] of the ini
Float Function GetSetting(String id) Global Native

; written into MCM/Settings/KnowYourSkyrim.ini, it holds the next time the inventory opens
Function SetSetting(String id, Float value) Global Native
