Den här mappen innehåller källkoden till klientprogrammet samt det förkompilerade
programmet. Den här filen beskriver kort hur koden öppnas och kompileras samt hur
programmet startas. Mer information om källkoden finns i den tekniska dokumentationen,
och information hur programmet används i användarmanualen.

 -------
|KÄLLKOD|
 -------
Källkoden är skriven i Visual Studio 2013 och öppnas genom att ladda filen Robotbil.sln.
För att programmet ska kunna kompileras måste OpenCV vara installerat på datorn. I
projektet har OpenCV 3.0 används, som finns att ladda ner på http://opencv.org. Projektet
är förinställt att leta i C:\-roten på datorn, så filen som laddas ner borde packas upp
till C:\opencv. Detta går dock att ändra i projektet genom att i Visual Studio höger-
klicka på projektet, välja Properties och sedan ändra två sökvägar.

Den första sökvägen som behöver ändras finns i "Additional Include Directories" som 
ligger under Configuration Properties -> C/C++ -> General. Den andra finns i
"Additional Library Directories" som ligger under Configuration Properties ->
Linker -> General.

Vidare kan även DLL-filen XInput9_1_0.dll behöva kopieras från programmappen till
C:\Windows\system32 om en sådan fil inte redan finns.

 -------
|PROGRAM|
 -------
För att köra programmet ska helt enkelt programmet MindBot.exe som ligger i program-
mappen. Alla nödvändiga filer ska redan finnas där, men ifall någon skulle saknas
går det att kopiera in från antingen programmappen eller ifrån mappen där OpenCV är
installerad.