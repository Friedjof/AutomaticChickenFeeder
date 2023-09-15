var rowCount = 0;
var currentId = 0;

var feed_on = false;

async function getTimers() {
    
    fetch('/get')
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

    document.getElementById('timers').getElementsByTagName('tbody')[0].appendChild(row);
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

function removeRow(id) {
    if (rowCount <= 1) {
        showNotification('warning', 'At least one timer must be present &#128679;', 3000);
        return;
    }

    document.getElementById('row-' + id).remove();

    showNotification('info', 'Line removed', 1000);

    rowCount--;
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
    fetch('/sleep')
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
    link.setAttribute('download', 'schedule.csv');
    document.body.appendChild(link);
    document.querySelector('#download').click();

    showNotification('info', 'Choose a location', 3000);
}

function importCSV() {
    showNotification('info', 'Choose a suitable CSV file', 3000);

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

                cols.pop();

                if (cols.length != 10) {
                    showNotification('error', 'Error importing the schedule (wrong format &#128269; )', 5000);
                    return;
                }

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
        showNotification('info', 'Feeding started &#127744;', 3000);
    else
        showNotification('info', 'Feeding stopped &#9940;', 3000);
    
    var feed_button = document.getElementById('feed-button');
    feed_button.innerHTML = "Feed manually (" + (feed_on ? "on" : "off") + ")";
}

window.onload = getTimers;