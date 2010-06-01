/**
 * Task, der die Unit repräsentieren soll.
 * Initialisiert und bedient den Comport Treiber
 * Initialisiert und bedient den Job Handler
 */
void vComportUnitTask( void *pvParameters );

/**
 * Callback Funktion für Eintreffen eines Jobs
 * Diese Funktion wird vom UDP Task als Job Handler aufgerufen
 * und muss den darin befindlichen Job in den Unit Task
 * synchronisieren
 */
void vComportUnitJobReceived(tUnitJob job);
