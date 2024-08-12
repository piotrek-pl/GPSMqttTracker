import QtQuick 2.15
import QtQuick.Controls 2.5

RangeSlider {
    id: slider
    from: 0
    to: 100
    first.value: 0  // Dolny ogranicznik
    second.value: 100 // Górny ogranicznik
    stepSize: 1

    anchors.fill: parent

    function setFirstValue(value) {
        console.log("Setting first.value to", value);
        first.value = value;
    }

    function setSecondValue(value) {
        console.log("Setting second.value to", value);
        second.value = value;
    }

    // Reagowanie na ruch dolnym ogranicznikiem
    first.onMoved: {
        console.log("First handle moved. New value: " + first.value);
        // Tutaj możesz dodać dodatkową logikę, którą chcesz wykonać
    }

    // Reagowanie na ruch górnym ogranicznikiem
    second.onMoved: {
        console.log("Second handle moved. New value: " + second.value);
        // Tutaj możesz dodać dodatkową logikę, którą chcesz wykonać
    }
}
