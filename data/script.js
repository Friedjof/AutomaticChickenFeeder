// Zähler für die Zeilen (Anzahl der Timer)
var rowCount = 0;
var currentId = 0;

var feed_on = false;

// Funktion, um Timer von der API abzurufen
async function getTimers() {
    // Erstelle einen leeren Timer
    
    fetch('/get')
        .then(response => response.json())
        .then(data => {
            // Überprüfen, ob Daten vorhanden sind
            if (!data) {
                showNotification('error', 'Fehler beim Laden des Zeitplans (keine Verbindung &#128268; )', 5000);
                return;
            }
            
            // Tabelle leeren
            document.getElementById('timers-body').innerHTML = '';
            
            rowCount = 0;
            currentId = 0;

            // Alle Timer durchgehen
            data["timers"].forEach(timer => {
                showTimer(currentId, timer);
                
                currentId++;
                rowCount++;
            });

            var feed = document.getElementById('feed');
            feed.value = data["feed"]["quantity"]
            
            showNotification('success', 'Zeitplan geladen', 3000);
        })
        .catch(error => {
            showNotification('error', 'Fehler beim Laden des Zeitplans (keine Verbindung &#128268; )', 5000);
        });
}

function showTimer(id, timer) {
    // Neue Zeile erstellen
    var row = document.createElement('tr');
    row.id = 'row-' + id;

    // Spalten erstellen
    var enabled = document.createElement('td');
    enabled.innerHTML = '<input type="checkbox" class="checked" ' + (timer.enabled ? 'checked' : '') + '>';
    row.appendChild(enabled);

    var name = document.createElement('td');
    name.innerHTML = '<input type="text" placeholder="Name" class="name" maxlength="10" value="' + timer.name + '">';
    row.appendChild(name);

    var time = document.createElement('td');
    time.innerHTML = '<input type="time" required class="time" value="' + timer.time + '">';
    row.appendChild(time);

    var monday = document.createElement('td');
    monday.innerHTML = '<input type="checkbox" class="mon" ' + (timer.days.monday ? 'checked' : '') + '>';
    row.appendChild(monday);

    var tuesday = document.createElement('td');
    tuesday.innerHTML = '<input type="checkbox" class="tue" ' + (timer.days.tuesday ? 'checked' : '') + '>';
    row.appendChild(tuesday);

    var wednesday = document.createElement('td');
    wednesday.innerHTML = '<input type="checkbox" class="wed" ' + (timer.days.wednesday ? 'checked' : '') + '>';
    row.appendChild(wednesday);

    var thursday = document.createElement('td');
    thursday.innerHTML = '<input type="checkbox" class="thu" ' + (timer.days.thursday ? 'checked' : '') + '>';
    row.appendChild(thursday);

    var friday = document.createElement('td');
    friday.innerHTML = '<input type="checkbox" class="fri" ' + (timer.days.friday ? 'checked' : '') + '>';
    row.appendChild(friday);

    var saturday = document.createElement('td');
    saturday.innerHTML = '<input type="checkbox" class="sat" ' + (timer.days.saturday ? 'checked' : '') + '>';
    row.appendChild(saturday);

    var sunday = document.createElement('td');
    sunday.innerHTML = '<input type="checkbox" class="sun" ' + (timer.days.sunday ? 'checked' : '') + '>';
    row.appendChild(sunday);

    var remove = document.createElement('td');
    remove.innerHTML = '<button onclick="removeRow(' + id + ')" class="remove">&#x1F5D1;</button>';
    remove.id = 'remove-' + id;
    row.appendChild(remove);

    // Zeile der Tabelle hinzufügen
    document.getElementById('timers').getElementsByTagName('tbody')[0].appendChild(row);
}

function addRow() {
    // max. 4 Zeilen
    if (rowCount >= 4) {
        showNotification('warning', 'Maximal 4 Timer sind möglich &#128679;', 3000);
        return;
    }

    // Neue Zeile erstellen
    var timer = {
        id: currentId,
        enabled: false,
        name: 'Timer ' + (currentId + 1),
        time: '12:00',
        days: {
            monday: false,
            tuesday: false,
            wednesday: false,
            thursday: false,
            friday: false,
            saturday: false,
            sunday: false
        }
    };

    showTimer(currentId, timer);

    rowCount++; // Zähler erhöhen
    currentId++; // ID erhöhen

    showNotification('info', 'Zeile hinzugefügt', 1000);
}

function removeRow(id) {
    if (rowCount <= 1) {
        showNotification('warning', 'Mindestens eine Timer muss vorhanden sein &#128679;', 3000);
        return;
    }

    // Zeile entfernen
    document.getElementById('row-' + id).remove();

    showNotification('info', 'Zeile entfernt', 1000);

    rowCount--; // Zähler verringern
}

async function saveTimers() {
    // Liste aller Zeilen
    var rows = document.getElementById('timers').getElementsByTagName('tbody')[0].getElementsByTagName('tr');

    // Wenn die Feed quantity kleiner als 0 ist oder keine Zahl ist, wird ein Fehler angezeigt
    if (parseInt(document.getElementById('feed').value) < 0 || isNaN(parseInt(document.getElementById('feed').value))) {
        showNotification('error', 'Fehler beim Speichern des Zeitplans (Futtermenge ungültig &#128290; )', 5000);
        return;
    }
    var feed_quantity = parseInt(document.getElementById('feed').value);

    // Liste für die Timer
    var timers = [];

    // Alle Zeilen durchgehen
    for (var i = 0; i < rows.length; i++) {
        // Timer erstellen
        var timer = {
            'enabled': rows[i].getElementsByTagName('input')[0].checked,
            'name': rows[i].getElementsByTagName('input')[1].value,
            'time': rows[i].getElementsByTagName('input')[2].value,
            'days': {
                'monday': rows[i].getElementsByTagName('input')[3].checked,
                'tuesday': rows[i].getElementsByTagName('input')[4].checked,
                'wednesday': rows[i].getElementsByTagName('input')[5].checked,
                'thursday': rows[i].getElementsByTagName('input')[6].checked,
                'friday': rows[i].getElementsByTagName('input')[7].checked,
                'saturday': rows[i].getElementsByTagName('input')[8].checked,
                'sunday': rows[i].getElementsByTagName('input')[9].checked
            }
        };

        // Timer der Liste hinzufügen
        timers.push(timer);
    }

    // Timer an die API senden
    await fetch('/set', {
        method: 'POST',
        headers: {
        'Accept': 'application/json',
        'Content-Type': 'application/json'
        },
        body: JSON.stringify({
            timers: timers,
            feed: {
                quantity: feed_quantity
            }
        })

    }).catch(error => {
        showNotification('error', 'Fehler beim Speichern des Zeitplans (keine Verbindung &#128268; )', 5000);
    }).then(response => {
        // Überprüfen, ob Daten vorhanden sind
        if (!response) {
            return;
        }

        showNotification('success', 'Zeitplan gespeichert &#128668;', 3000);
    });
}

function showNotification(notType, msg, showTime) {
    // Erstelle das Benachrichtigungsdiv
    const notification = document.createElement('div');
    notification.className = `notification ${notType}`;

    var emoji = '';
    if (notType == 'success') {
        emoji = '&#128232;';
    } else if (notType == 'info') {
        emoji = '&#128276;';
    } else if (notType == 'warning') {
        emoji = '&#128293;';
    } else if (notType == 'error') {
        emoji = '&#127785;';
    }

    // Füge die Nachricht zum Benachrichtigungsdiv hinzu (emoji + Nachricht)
    notification.innerHTML = emoji + ' ' + msg;

    // Füge die Benachrichtigung zum Container hinzu
    const container = document.getElementById('notification-container');
    container.appendChild(notification);

    // Verberge die Benachrichtigung nach showTime Millisekunden
    setTimeout(() => {
        notification.style.opacity = '0';
        setTimeout(() => {
            container.removeChild(notification);
        }, 500); // Warte 0,5 Sekunden, bevor die Benachrichtigung entfernt wird
    }, showTime);

    // Durch einen Klick auf die Benachrichtigung wird diese sofort entfernt
    notification.addEventListener('click', () => {
        notification.style.opacity = '0';
        setTimeout(() => {
            container.removeChild(notification);
        }, 100); // Warte 0,1 Sekunden, bevor die Benachrichtigung entfernt wird
    });
}

function sleep_mode() {
    fetch('/sleep')
        .then(data => {
            // Zeitverzögert runter zählen von 10
            var count = 10;
            var counter = setInterval(timer, 1000);
            function timer() {
                count = count - 1;
                if (count <= 0) {
                    clearInterval(counter);
                    showNotification('success', 'Schlafmodus aktiviert &#128564;', 3000);
                    disableInputs();
                    return;
                }

                showNotification('info', 'Schlafmodus aktiviert in ' + count + ' Sekunden &#128564;', 1000);
            }
        })
        .catch(error => {
            showNotification('error', 'Fehler beim Aktivieren des Schlafmodus (keine Verbindung &#128268; )', 5000);
        });
}

function export2CSV() {
    const table = document.getElementById('timers-body');
    const rows = table.getElementsByTagName('tr');

    // Wenn die Zeilenanzahl größer als 4 ist, wird ein Fehler angezeigt
    if (rows.length > 4) {
        showNotification('warning', 'Maximal 4 Timer sind möglich &#128679;', 3000);
        return;
    }

    // Wenn die Zeilenanzahl kleiner als 1 ist, wird ein Fehler angezeigt
    if (rows.length < 1) {
        showNotification('warning', 'Mindestens eine Timer muss vorhanden sein &#128679;', 3000);
        return;
    }

    var csv = 'enabled;name;time;mon;tue;wed;thu;fri;sat;sun\n';
    var seperator = ';';

    for (var i = 0; i < rows.length; i++) {
        var row = rows[i];
        var cols = row.getElementsByTagName('td');

        if (cols[0].getElementsByClassName('checked')[0].checked)
            csv += '1' + seperator;
        else
            csv += '0' + seperator;

        csv += cols[1].getElementsByClassName('name')[0].value;
        csv += seperator;

        csv += cols[2].getElementsByClassName('time')[0].value;
        csv += seperator;

        var weekdays = ['mon', 'tue', 'wed', 'thu', 'fri', 'sat', 'sun'];

        weekdays.forEach(weekday => {
            if (cols[3 + weekdays.indexOf(weekday)].getElementsByClassName(weekday)[0].checked)
                csv += '1' + seperator;
            else
                csv += '0' + seperator;
        });

        csv += '\n';
    }

    var link = document.createElement('a');
    link.id = 'download';
    link.setAttribute('href', 'data:text/csv;charset=utf-8,' + encodeURIComponent(csv));
    link.setAttribute('download', 'Zeitplan.csv');
    document.body.appendChild(link);
    document.querySelector('#download').click();

    showNotification('info', 'Wähle einen Speicherort aus', 3000);
}

function importCSV() {
    showNotification('info', 'Wähle eine passende CSV-Datei aus', 3000);

    var input = document.createElement('input');
    input.type = 'file';
    input.accept = '.csv';
    input.onchange = function() {
        var file = input.files[0];
        var reader = new FileReader();
        reader.readAsText(file);
        reader.onload = function() {
            var csv = reader.result;
            var rows = csv.split('\n');

            // Lösche die erste Zeile (Überschrift)
            rows.shift();

            // Lösche leere Zeilen
            rows = rows.filter(row => row != '');

            // Tabelle leeren
            document.getElementById('timers-body').innerHTML = '';

            currentId = 0;
            rowCount = 0;

            // Wenn die Zeilenanzahl größer als 4 ist, wird ein Fehler angezeigt
            if (rows.length > 4) {
                showNotification('error', 'Maximal 4 Timer sind möglich &#128679;', 3000);
                return;
            }
            
            // Wenn die Zeilenanzahl kleiner als 1 ist, wird ein Fehler angezeigt
            if (rows.length < 1) {
                showNotification('error', 'Mindestens eine Timer muss vorhanden sein &#128679;', 3000);
                return;
            }

            // Alle Zeilen durchgehen
            for (var i = 0; i < rows.length; i++) {
                var cols = rows[i].split(';');

                // Lösche die letzte Spalte (Leerzeichen)
                cols.pop();

                if (cols.length != 10) {
                    showNotification('error', 'Fehler beim Importieren des Zeitplans (falsches Format &#128269; )', 5000);
                    return;
                }

                // Timer erstellen
                var timer = {
                    'enabled': cols[0] == '1',
                    'name': cols[1],
                    'time': cols[2],
                    'days': {
                        'monday': cols[3] == '1',
                        'tuesday': cols[4] == '1',
                        'wednesday': cols[5] == '1',
                        'thursday': cols[6] == '1',
                        'friday': cols[7] == '1',
                        'saturday': cols[8] == '1',
                        'sunday': cols[9] == '1'
                    }
                };

                showTimer(currentId, timer);

                currentId++;
                rowCount++;
            };

            showNotification('success', 'Zeitplan wurde erfolgreich importiert &#128668;', 3000);
        };
    };
    input.click();
}

// Diese Funktion deaktiviert alle inputs, checkboxen und buttons und zeigt einen Ladebildschirm an
function disableInputs() {
    // Alle inputs, checkboxen und buttons deaktivieren
    var inputs = document.getElementsByTagName('input');
    for (var i = 0; i < inputs.length; i++) {
        inputs[i].disabled = true;
    }

    var buttons = document.getElementsByTagName('button');
    for (var i = 0; i < buttons.length; i++) {
        buttons[i].disabled = true;
    }

    // Ladebildschirm anzeigen
    showLoadingSpinner();
}

function showLoadingSpinner() {
    // Erstelle ein Overlay-Element
    var overlay = document.createElement('div');
    overlay.className = 'loading-overlay';

    // Erstelle einen Ladekreis im Overlay
    var spinner = document.createElement('div');
    spinner.className = 'loading-spinner';
    overlay.appendChild(spinner);

    // Schriftzug hinzufügen
    var text = document.createElement('p');
    text.className = 'loading-text';
    text.innerHTML = 'Ich schlafe nun &#128564;\n<br>Drücke den Knopf, um mich wieder aufzuwecken.';
    overlay.appendChild(text);

    // Füge das Overlay zum Body hinzu
    document.body.appendChild(overlay);
}

function hideLoadingSpinner() {
    // Entferne das Overlay-Element
    var overlay = document.querySelector('.loading-overlay');
    if (overlay) {
        document.body.removeChild(overlay);
    }
}

function feedManual() {
    feed_on = !feed_on;
    
    fetch('/feed', {
        method: 'POST',
        headers: {
            'Accept': 'application/json',
            'Content-Type': 'application/json'
        },
        body: JSON.stringify({
            on: feed_on
        })
    });

    if (feed_on)
        showNotification('info', 'Fütterung gestartet &#127744;', 3000);
    else
        showNotification('info', 'Fütterung gestoppt &#9940;', 3000);
    
    var feed_button = document.getElementById('feed-button');
    feed_button.innerHTML = "Manuell füttern (" + (feed_on ? "an" : "aus") + ")";
}

window.onload = getTimers;