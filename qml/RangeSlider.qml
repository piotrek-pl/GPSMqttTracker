import QtQuick 2.15
import QtQuick.Controls 2.15

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
}
