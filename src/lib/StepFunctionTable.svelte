<script lang="ts">

    import {type Writable, derived, type Readable} from "svelte/store";
    import {Input} from "$lib/components/ui/input";

    type Point = [number, number]
    export let stepFunction: Writable<[Point]>;

    const GRADE: number = 0, YIELD: number = 1;
    const ALLOWED_CHARS: Set<String> = new Set<String>("0123456789.")


    const stepFunctionBounds: Readable<[[], []]> = derived(stepFunction, ($stepFunction) => {
        let paddedPoints = [[0.0, 0.0], ...$stepFunction, [1.0, 1.0]];
        let bounds = [];
        for (let i = 1; i < paddedPoints.length - 1; i++) {
            const [g1, y1] = paddedPoints[i - 1];
            const [g2, y2] = paddedPoints[i + 1];
            bounds.push([[g1, g2], [y1, y2]]);
        }
        return bounds;
    })


    function connectLastGrade() {
        const secondToLastIndex = $stepFunction.length - 2;
        const lastIndex = $stepFunction.length - 1;
        $stepFunction[lastIndex][0] = $stepFunction[secondToLastIndex][0];
    }

    function innputGuard(event: KeyboardEvent) {
        if (!ALLOWED_CHARS.has(event.key)) event.preventDefault();
    }

    function isLast(index: number, offset: number = 0): boolean {
        return index === $stepFunction.length - 1 - offset;
    }

</script>

<table>
    <thead>
    <tr>
        <th>Grade [1]</th>
        <th>Yield [1]</th>
    </tr>
    </thead>
    <tbody>
    {#each {length: $stepFunction.length} as _, i}
        <tr>
            <td><Input
                    type="number"
                    bind:value={$stepFunction[i][GRADE]}
                    disabled={isLast(i)}
                    on:change={isLast(i, 1) ? connectLastGrade: () => {}}
                    min={$stepFunctionBounds[i][GRADE][0]}
                    max={$stepFunctionBounds[i][GRADE][1]}
                    class="border-accent-foreground"
                    on:keypress={innputGuard}
            /></td>
            <td><Input
                    type="number"
                    bind:value={$stepFunction[i][YIELD]}
                    disabled={isLast(i)}
                    min={$stepFunctionBounds[i][YIELD][0]}
                    max={$stepFunctionBounds[i][YIELD][1]}
                    on:keypress={innputGuard}
            />
            </td>
        </tr>


    {/each}
    </tbody>
</table>

