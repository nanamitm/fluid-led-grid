import QtQuick

Canvas {
    id: root

    property string icon: "close"
    property color strokeColor: "#88aacc"
    property real lineWidth: 2.2

    implicitWidth: 24
    implicitHeight: 24

    onIconChanged: requestPaint()
    onStrokeColorChanged: requestPaint()
    onLineWidthChanged: requestPaint()
    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()

    onPaint: {
        const ctx = getContext("2d")
        const w = width
        const h = height
        const cx = w / 2
        const cy = h / 2
        const s = Math.min(w, h)

        ctx.reset()
        ctx.clearRect(0, 0, w, h)
        ctx.strokeStyle = strokeColor
        ctx.fillStyle = strokeColor
        ctx.lineWidth = lineWidth
        ctx.lineCap = "round"
        ctx.lineJoin = "round"

        if (icon === "close") {
            const p = s * 0.28
            ctx.beginPath()
            ctx.moveTo(p, p)
            ctx.lineTo(w - p, h - p)
            ctx.moveTo(w - p, p)
            ctx.lineTo(p, h - p)
            ctx.stroke()
            return
        }

        if (icon === "gear") {
            const inner = s * 0.19
            const outer = s * 0.34
            const toothInner = s * 0.38
            const toothOuter = s * 0.45

            for (let i = 0; i < 8; ++i) {
                const a = i * Math.PI / 4
                ctx.beginPath()
                ctx.moveTo(cx + Math.cos(a) * toothInner, cy + Math.sin(a) * toothInner)
                ctx.lineTo(cx + Math.cos(a) * toothOuter, cy + Math.sin(a) * toothOuter)
                ctx.stroke()
            }

            ctx.beginPath()
            ctx.arc(cx, cy, outer, 0, Math.PI * 2)
            ctx.stroke()
            ctx.beginPath()
            ctx.arc(cx, cy, inner, 0, Math.PI * 2)
            ctx.stroke()
            return
        }

        if (icon === "reset") {
            const r = s * 0.30
            ctx.beginPath()
            ctx.arc(cx, cy, r, Math.PI * 0.18, Math.PI * 1.65, false)
            ctx.stroke()

            const tipX = cx + Math.cos(Math.PI * 1.65) * r
            const tipY = cy + Math.sin(Math.PI * 1.65) * r
            ctx.beginPath()
            ctx.moveTo(tipX, tipY)
            ctx.lineTo(tipX + s * 0.01, tipY + s * 0.18)
            ctx.lineTo(tipX - s * 0.16, tipY + s * 0.08)
            ctx.closePath()
            ctx.fill()
        }
    }
}
