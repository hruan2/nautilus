#!/bin/bash
export PATH=/home/pdinda/HANDOUT/tools/bin:$PATH

MODE=$1
MARKER="NAUTILUS_BOOT_COMPLETE"
RUNS=5
ROM=/files10/cs446-2026-spring/abn7489/coreboot/build/coreboot.rom
CBFS=/files10/cs446-2026-spring/abn7489/cbfstool

if [ "$MODE" = "coreboot" ]; then
    $CBFS $ROM remove -n fallback/payload
    $CBFS $ROM add-payload -f /home/abn7489/nautilus/nautilus.bin -n fallback/payload
fi

for i in $(seq 1 $RUNS); do
    TMPOUT=$(mktemp)

    if [ "$MODE" = "coreboot" ]; then
        qemu-system-x86_64 -M q35 -bios $ROM \
            -serial stdio -display none 2>/dev/null >"$TMPOUT" &
    else
        qemu-system-x86_64 -m 2048 -serial stdio -display none \
            -cdrom /home/abn7489/nautilus/nautilus.iso 2>/dev/null >"$TMPOUT" &
    fi
    QEMU_PID=$!
    START=$(date +%s%N)

    while true; do
        if grep -q "$MARKER" "$TMPOUT" 2>/dev/null; then
            END=$(date +%s%N)
            break
        fi
        sleep 0.05
    done

    kill $QEMU_PID 2>/dev/null
    wait $QEMU_PID 2>/dev/null
    rm -f "$TMPOUT"

    MS=$(( (END - START) / 1000000 ))
    echo "Run $i: ${MS} ms"
done
