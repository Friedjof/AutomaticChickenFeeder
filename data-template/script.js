var rowCount = 0;
var currentId = 0;

var url = 'http://localhost:5000'

var timers = {}


async function getTimers() {
    fetch(url + '/get')
        .then(response => response.json())
        .then(data => {
            if (!data) {
                showNotification('error', 'Error loading the schedule (no connection &#128268; )', 5000);
                return;
            }
            
            document.getElementById('timers-body').innerHTML = '';
            
            rowCount = 0;
            currentId = 0;

            data["timers"].forEach(timer => {
                showTimer(currentId, timer);
                
                currentId++;
                rowCount++;
            });

            var feed = document.getElementById('feed');
            feed.value = data["feed"]["quantity"]
            
            showNotification('success', 'Schedule loaded', 3000);
        })
        .catch(error => {
            showNotification('error', 'Error loading the schedule (no connection &#128268; )', 5000);
        });
}

function showTimer(id, timer) {
    timers[id] = timer;

    var row = document.createElement('tr');
    row.id = 'row-' + id;

    var enabled = document.createElement('td');
    enabled.innerHTML = '<input type="checkbox" class="checked" ' + (timer.enabled ? 'checked' : '') + '>';
    row.appendChild(enabled);

    var name = document.createElement('td');
    name.innerHTML = '<input type="text" placeholder="Name" class="name" maxlength="10" value="' + timer.name + '">';
    row.appendChild(name);

    var time = document.createElement('td');
    time.innerHTML = '<input type="time" required class="time" value="' + timer.time + '">';
    row.appendChild(time);

    var popup_button = document.createElement('td');
    popup_button.innerHTML = '<button onclick="showPopup(' + id + ')" class="input_buttons">&#128336;</button>';
    popup_button.id = 'popup-' + id;
    row.appendChild(popup_button);

    var remove = document.createElement('td');
    remove.innerHTML = '<button onclick="removeRow(' + id + ')" class="input_buttons">&#x1F5D1;</button>';
    remove.id = 'remove-' + id;
    row.appendChild(remove);

    document.getElementById('timers').getElementsByTagName('tbody')[0].appendChild(row);
}

function showPopup(id) {
    // Erstellen Sie ein Overlay
    var overlay = document.createElement('div');
    overlay.className = 'overlay';
    overlay.onclick = function() {
        closePopup(overlay, modal);
    }

    document.body.appendChild(overlay);

    // Erstellen Sie ein Div für das Modal
    var modal = document.createElement('div');
    modal.className = 'modal';

    // Erstellen Sie einen Schließen-Button für das Modal
    var closeButton = document.createElement('button');
    closeButton.innerHTML = '&#x2715;';
    closeButton.className = 'close-button';
    closeButton.onclick = function() {
        closePopup(overlay, modal);
    }

    modal.appendChild(closeButton);

    // Fügen Sie einen Text zum Modal hinzu
    var text = document.createElement('h2');
    text.className = 'schedule-header';
    text.textContent = '⏰ ' + timers[id].time + ' - ' + timers[id].name;
    modal.appendChild(text);

    // Füge die Wochentage hinzu
    var daysOfWeek = ['monday', 'tuesday', 'wednesday', 'thursday', 'friday', 'saturday', 'sunday'];
    var dayNames = ['Monday', 'Tuesday', 'Wednesday', 'Thursday', 'Friday', 'Saturday', 'Sunday'];

    var days = document.createElement('div');
    days.className = 'days';
    modal.appendChild(days);
    
    daysOfWeek.forEach((day, index) => {
        var dayContainer = document.createElement('div');
        dayContainer.className = 'day-container';
    
        var dayLabel = document.createElement('label');
        dayLabel.textContent = dayNames[index];
        dayLabel.htmlFor = day + '-' + id;
        dayLabel.className = 'day-label';
    
        var dayCheckbox = document.createElement('input');
        dayCheckbox.type = 'checkbox';
        dayCheckbox.id = day + '-' + id;
        dayCheckbox.checked = timers[id].days[day];
        dayCheckbox.onclick = function() {
            timers[id].days[day] = !timers[id].days[day];
        }
    
        dayContainer.appendChild(dayLabel);
        dayContainer.appendChild(dayCheckbox);
        days.appendChild(dayContainer);
    });
    


    // Fügen Sie das Modal zum Body hinzu
    document.body.appendChild(modal);
}

function closePopup(overlay, modal) {
    document.body.removeChild(modal);
    document.body.removeChild(overlay);
}

function removeRow(id) {
    if (rowCount <= 1) {
        showNotification('warning', 'At least one timer must be present &#128679;', 3000);
        return;
    }

    document.getElementById('row-' + id).remove();
    delete timers[id];

    console.log(timers);

    showNotification('info', 'Line removed', 1000);

    rowCount--;
}

function addRow() {
    if (rowCount >= 4) {
        showNotification('warning', 'A maximum of 4 timers are possible &#128679;', 3000);
        return;
    }

    var timer = {
        id: currentId,
        enabled: false,
        name: 'Alert ' + (currentId + 1),
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

    rowCount++;
    currentId++;

    showNotification('info', 'Line added', 1000);
}

async function saveTimers() {
    var rows = document.getElementById('timers').getElementsByTagName('tbody')[0].getElementsByTagName('tr');

    if (parseInt(document.getElementById('feed').value) < 0 || isNaN(parseInt(document.getElementById('feed').value))) {
        showNotification('error', 'Error when saving the schedule (feed quantity invalid &#128290; )', 5000);
        return;
    }
    var feed_quantity = parseInt(document.getElementById('feed').value);

    var timers = [];

    for (var i = 0; i < rows.length; i++) {
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

        timers.push(timer);
    }

    await fetch(url + '/set', {
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
        showNotification('error', 'Error when saving the schedule (no connection &#128268; )', 5000);
    }).then(response => {
        if (!response) {
            return;
        }

        showNotification('success', 'Schedule saved &#128668;', 3000);
    });
}

function showNotification(notType, msg, showTime) {
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

    notification.innerHTML = emoji + ' ' + msg;

    const container = document.getElementById('notification-container');
    container.appendChild(notification);

    setTimeout(() => {
        notification.style.opacity = '0';
        setTimeout(() => {
            container.removeChild(notification);
        }, 500);
    }, showTime);

    notification.addEventListener('click', () => {
        notification.style.opacity = '0';
        setTimeout(() => {
            container.removeChild(notification);
        }, 100);
    });
}

function sleep_mode() {
    fetch(url + '/sleep')
        .then()
        .catch(error => {
            showNotification('error', 'Error when activating the sleep mode (no connection &#128268; )', 5000);
        });
    
    var count = 10;
    var counter = setInterval(timer, 1000);
    function timer() {
        count = count - 1;
        if (count <= 0) {
            clearInterval(counter);
            showNotification('success', 'Sleep mode activated &#128564;', 3000);
            disableInputs();
            return;
        }

        showNotification('info', 'Sleep mode enabled in ' + count + ' sec &#128564;', 1000);
    }
}

function export2CSV() {
    const table = document.getElementById('timers-body');
    const rows = table.getElementsByTagName('tr');

    if (rows.length > 4) {
        showNotification('warning', 'A maximum of 4 timers are possible &#128679;', 3000);
        return;
    }

    if (rows.length < 1) {
        showNotification('warning', 'At least one timer must be present &#128679;', 3000);
        return;
    }

    var csv = 'enabled;name;time;mon;tue;wed;thu;fri;sat;sun\n';
    var seperator = ';';

    Object.values(timers).forEach(timer => {
        csv += timer.enabled ? '1' : '0';
        csv += seperator;
        csv += timer.name;
        csv += seperator;
        csv += timer.time;
        csv += seperator;
        csv += timer.days.monday ? '1' : '0';
        csv += seperator;
        csv += timer.days.tuesday ? '1' : '0';
        csv += seperator;
        csv += timer.days.wednesday ? '1' : '0';
        csv += seperator;
        csv += timer.days.thursday ? '1' : '0';
        csv += seperator;
        csv += timer.days.friday ? '1' : '0';
        csv += seperator;
        csv += timer.days.saturday ? '1' : '0';
        csv += seperator;
        csv += timer.days.sunday ? '1' : '0';
        csv += '\n';
    });

    var link = document.createElement('a');
    link.id = 'download';
    link.setAttribute('href', 'data:text/csv;charset=utf-8,' + encodeURIComponent(csv));
    link.setAttribute('download', 'schedule.csv');
    document.body.appendChild(link);
    document.querySelector('#download').click();

    showNotification('info', 'Choose a location', 3000);
}

function importCSV() {
    showNotification('info', 'Choose a suitable CSV file', 3000);

    timers = {};

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

            rows.shift();

            rows = rows.filter(row => row != '');

            document.getElementById('timers-body').innerHTML = '';

            currentId = 0;
            rowCount = 0;

            if (rows.length > 4) {
                showNotification('error', 'A maximum of 4 timers are possible &#128679;', 3000);
                return;
            }
            
            if (rows.length < 1) {
                showNotification('error', 'At least one timer must be present &#128679;', 3000);
                return;
            }

            for (var i = 0; i < rows.length; i++) {
                var cols = rows[i].split(';');

                if (cols.length != 10) {
                    showNotification('error', 'Error importing the schedule (wrong format &#128269; )', 5000);
                    return;
                }

                timers[currentId] = {
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

                showTimer(currentId, timers[currentId]);

                currentId++;
                rowCount++;
            };

            showNotification('success', 'Schedule was imported successfully &#128668;', 3000);
        };
    };
    input.click();
}

function disableInputs() {
    var inputs = document.getElementsByTagName('input');
    for (var i = 0; i < inputs.length; i++) {
        inputs[i].disabled = true;
    }

    var buttons = document.getElementsByTagName('button');
    for (var i = 0; i < buttons.length; i++) {
        buttons[i].disabled = true;
    }

    showLoadingSpinner();
}

function showLoadingSpinner() {
    var overlay = document.createElement('div');
    overlay.className = 'loading-overlay';

    var spinner = document.createElement('div');
    spinner.className = 'loading-spinner';
    overlay.appendChild(spinner);

    var text = document.createElement('p');
    text.className = 'loading-text';
    text.innerHTML = 'I am now asleep &#128564;\n<br>Press the button to wake me up again.';
    overlay.appendChild(text);

    document.body.appendChild(overlay);
}

function hideLoadingSpinner() {
    var overlay = document.querySelector('.loading-overlay');
    if (overlay) {
        document.body.removeChild(overlay);
    }
}

function feedManual() {
    fetch(url + '/feed', {
        method: 'POST',
        headers: {
            'Accept': 'application/json',
            'Content-Type': 'application/json'
        },
        body: JSON.stringify({})
    });

    showNotification('info', 'Feeding started &#127744;', 3000);
    
    var feed_button = document.getElementById('feed-button');
    feed_button.innerHTML = "Feed manually";
}

// Display current date and time
function displayCurrentDateTime() {
    fetch(url + '/time')
        .then(response => response.json())
        .then(data => {
            if (!data) {
                return;
            }

            var date = new Date(data["year"], data["month"] - 1, data["day"], data["hour"], data["minute"], data["second"]);
            var date_options = { weekday: 'long', year: 'numeric', month: 'long', day: 'numeric' };
            var time_options = { hour: '2-digit', minute: '2-digit', second: '2-digit' };

            // Display date and time
            document.getElementById('date-text').innerHTML = date.toLocaleDateString('en-GB', date_options);
            document.getElementById('time-text').innerHTML = date.toLocaleTimeString('en-GB', time_options);
        }).catch(error => {
            showNotification('error', 'Error loading the current date and time (no connection &#128268; )', 5000);
        });
}

// Get remaining time until sleep
async function getRemainingAutoSleepTime() {
    fetch(url + '/autosleep')
        .then(response => response.json())
        .then(data => {
            if (!data) {
                return;
            }

            var remaining = data["remaining"];
            var remaining_text = document.getElementById('autosleep-remaining-time');
            remaining_text.innerHTML = remaining;
        }
    ).catch(error => {
        showNotification('error', 'Error loading the remaining time until sleep (no connection &#128268; )', 5000);
    }
    );
}

function setCurrentTime() {
    var currentDateTime = new Date();
    var data = {
        'y': currentDateTime.getFullYear(),
        'm': currentDateTime.getMonth() + 1,
        'd': currentDateTime.getDate(),
        'h': currentDateTime.getHours(),
        'min': currentDateTime.getMinutes(),
        's': currentDateTime.getSeconds()
    };

    fetch(url + '/rtc', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json',
        },
        body: JSON.stringify(data),
    })
    .then(response => {
        if (!response.ok) {
            throw new Error('Network response was not ok');
        }
        return response.text();
    })
    .catch((error) => {
        console.error('Error:', error);
    });
}

function onloadFunction() {
    setCurrentTime();
    getTimers();
}

setInterval(displayCurrentDateTime, 1000);
setInterval(getRemainingAutoSleepTime, 1000);

window.onload = onloadFunction;