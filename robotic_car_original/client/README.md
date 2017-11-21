Den h�r mappen inneh�ller k�llkoden till klientprogrammet samt det f�rkompilerade
programmet. Den h�r filen beskriver kort hur koden �ppnas och kompileras samt hur
programmet startas. Mer information om k�llkoden finns i den tekniska dokumentationen,
och information hur programmet anv�nds i anv�ndarmanualen.

 -------
|K�LLKOD|
 -------
K�llkoden �r skriven i Visual Studio 2013 och �ppnas genom att ladda filen Robotbil.sln.
F�r att programmet ska kunna kompileras m�ste OpenCV vara installerat p� datorn. I
projektet har OpenCV 3.0 anv�nds, som finns att ladda ner p� http://opencv.org. Projektet
�r f�rinst�llt att leta i C:\-roten p� datorn, s� filen som laddas ner borde packas upp
till C:\opencv. Detta g�r dock att �ndra i projektet genom att i Visual Studio h�ger-
klicka p� projektet, v�lja Properties och sedan �ndra tv� s�kv�gar.

Den f�rsta s�kv�gen som beh�ver �ndras finns i "Additional Include Directories" som 
ligger under Configuration Properties -> C/C++ -> General. Den andra finns i
"Additional Library Directories" som ligger under Configuration Properties ->
Linker -> General.

Vidare kan �ven DLL-filen XInput9_1_0.dll beh�va kopieras fr�n programmappen till
C:\Windows\system32 om en s�dan fil inte redan finns.

 -------
|PROGRAM|
 -------
F�r att k�ra programmet ska helt enkelt programmet MindBot.exe som ligger i program-
mappen. Alla n�dv�ndiga filer ska redan finnas d�r, men ifall n�gon skulle saknas
g�r det att kopiera in fr�n antingen programmappen eller ifr�n mappen d�r OpenCV �r
installerad.