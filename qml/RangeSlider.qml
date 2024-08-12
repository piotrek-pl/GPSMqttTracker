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

    function resetValues() {
        setFirstValue(0);
        setSecondValue(86400);
    }

    function setFirstValue(value) {
        console.log("Setting first.value to", value);
        first.value = value;
        firstTimeText.text = formatTime(first.value);
        updateVisibleRoutes();
    }

    function setSecondValue(value) {
        console.log("Setting second.value to", value);
        second.value = value;
        secondTimeText.text = formatTime(second.value);
        updateVisibleRoutes();
    }

    function formatTime(seconds) {
        var hours = Math.floor(seconds / 3600);
        var minutes = Math.floor((seconds % 3600) / 60);
        var secs = seconds % 60;
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
            console.log("mainWindow is not defined");
        }
    }

    // Tekst nad pierwszym suwakiem
    Text {
        id: firstTimeText
        text: slider.formatTime(slider.first.value)
        anchors.bottom: slider.top
        anchors.bottomMargin: 5
        anchors.horizontalCenter: slider.first.handle.horizontalCenter
        font.pointSize: 12
        color: "black"
    }

    // Tekst pod drugim suwakiem
    Text {
        id: secondTimeText
        text: slider.formatTime(slider.second.value)
        anchors.top: slider.bottom
        anchors.topMargin: 5
        anchors.horizontalCenter: slider.second.handle.horizontalCenter
        font.pointSize: 12
        color: "black"
    }
}
