import QtQuick 2.15
import QtQuick.Controls 2.5

RangeSlider {
    id: slider
    from: 0
    to: 86400
    first.value: 0
    second.value: 86400
    stepSize: 1

    anchors.fill: parent
    anchors.leftMargin: 12   // Mały margines z lewej strony
    anchors.rightMargin: 12  // Mały margines z prawej strony

    function resetValues() {
        setFirstValue(0);
        setSecondValue(86400);
    }

    function setFirstValue(value) {
        console.log("Ustawianie first.value na", value);
        first.value = value;
        firstTimeText.text = formatTime(first.value);
        updateVisibleRoutes();
    }

    function setSecondValue(value) {
        console.log("Ustawianie second.value na", value);
        second.value = value;
        secondTimeText.text = formatTime(second.value);
        updateVisibleRoutes();
    }

    function formatTime(seconds) {
        seconds = Math.max(0, seconds - 1);  // Odejmowanie 1 sekundy i upewnienie się, że nie spadniemy poniżej 0
        var hours = Math.floor(seconds / 3600);
        var minutes = Math.floor((seconds % 3600) / 60);
        var secs = Math.floor(seconds % 60);
        return (hours < 10 ? "0" : "") + hours + ":" +
               (minutes < 10 ? "0" : "") + minutes + ":" +
               (secs < 10 ? "0" : "") + secs;
    }

    // Reagowanie na ruch dolnym ogranicznikiem
    first.onMoved: {
        firstTimeText.text = formatTime(first.value);
        updateVisibleRoutes();
    }

    // Reagowanie na ruch górnym ogranicznikiem
    second.onMoved: {
        secondTimeText.text = formatTime(second.value);
        updateVisibleRoutes();
    }

    function updateVisibleRoutes() {
        if (typeof mainWindow !== "undefined") {
            mainWindow.updateRoutes(first.value, second.value);
        } else {
            console.log("mainWindow nie jest zdefiniowany");
        }
    }

    // Tekst nad pierwszym suwakiem (lewy uchwyt)
    Text {
        id: firstTimeText
        text: slider.formatTime(slider.first.value)
        anchors.bottom: slider.first.handle.top
        anchors.bottomMargin: 3
        anchors.horizontalCenter: slider.first.handle.horizontalCenter
        font.pointSize: 7  // Zmniejszono rozmiar tekstu
        color: "black"
    }

    // Tekst pod drugim suwakiem (prawy uchwyt)
    Text {
        id: secondTimeText
        text: slider.formatTime(slider.second.value)
        anchors.top: slider.second.handle.bottom
        anchors.topMargin: 3
        anchors.horizontalCenter: slider.second.handle.horizontalCenter
        font.pointSize: 7  // Zmniejszono rozmiar tekstu
        color: "black"
    }
}
