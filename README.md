# Hack_Lan
Projekt sterownika protokołu i filtra karty sieciowej, wraz z biblioteką do instalacji i obsługi sterowników w systemie. 
FDriver_Lan - sterownik filtra. PDriver_Lan - sterownik protokołu. hckl - bilioteka do obsługi i instalacji sterowników w systemie.
HLFInstaller i HLPInstaller - dodadkowe sterowniki zaprojektowane w cleu ominięcia zabezpieczeń np. programów antywirusowych podczas instalacji i kopiowania
plików sterownika protokolu i filtra do katalogu systemowego. Dzięki temu instalacja i dezinstalacji sterowników w systemie jest w pełni zautomatyzowana za pomocą
funkcji z dołączonej bilioteki ( InstallFilterDriver(...),InstallProtocolDriver(...) oraz StopAndUinstallProtocolDriver(...), StopAndUinstallFilterDriver(...) ). 
hlanfilter.inf i hlanprotocol.inf - pliki instalatora. Dzieki zainstalowanym sterownikom protokołu i filtra można projektować zaawansowane aplikacje sieciowe. 
Umożliwiają one przechwytywanie oraz wysyłanie pakietów. Sterownik filtra ma możliwość przechwycić pakiety zarówno odbierane jak i wysyłane. 
Bilioteka zostałą zaprojektowana tak aby dawała możliwość ustawiania nowych wątków w celu przesyłania przechwyconych pakietów do aplikacji użytkownika. 
Dzięki zaprojektowanej synchronizacji sterowniki są w stanie obsłużyć wile takich aplikacji.
