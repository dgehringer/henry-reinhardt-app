<script lang="ts">

    import {type Writable, derived, type Readable} from "svelte/store";
    import {Input} from "$lib/components/ui/input";
    import * as Popover from "$lib/components/ui/popover";
    import {Button} from "$lib/components/ui/button";


    const GRADE: number = 0, YIELD: number = 1, OFFSET: number = 0.001;

    type StepFunction = number[][];
    type Bounds = number[][][];

    let stepFunction: StepFunction = [[0.25, 0.0], [0.5, 0.5], [0.75, 0.75], [0.75, 1.0]];
    let firstGrade = 0.125;
    let finalGrade = 0.925;

    function computeBounds(s: StepFunction): Bounds {
        const off = 0.01;
        let pp = [[0.0, 0.0], ...s, [1.0, 1.0]];
        const [first_grade, first_yield] = s[0]
        let b = [[[0.0, first_grade], [0.0, first_yield]]];
        for (let i = 2; i < pp.length - 1; i++) {
            const [g1, y1] = pp[i - 1];
            const [g2, y2] = pp[i + 1];
            const gspan = g2 - g1;
            const yspan = y2 - y1;
            b.push([
                [g1 + (gspan * off), g2 - (gspan * off)],
                [y1 + (yspan * off), y2 - (yspan * off)]
            ]);
        }
        const [last_grade, last_yield] = s[s.length - 2];
        b[s.length - 2] = [[last_grade, 1.0], [last_yield, 1.0]];
        b[s.length - 1] = [[last_grade, 1.0], [last_yield, 1.0]];

        return b;
    }

    function isMonotonous(vals: number[]): boolean[] {
        const validity = vals.map(() => true);
        for (let i = 0; i < vals.length - 1; i++) {
            const a = vals[i];
            const b = vals[i + 1];
            if (a + OFFSET > b && !isLast(i, 1)) {
                validity[i] = false;
                validity[i + 1] = false;
                i++;
            }
        }
        return validity;
    }

    function computeValidity(s: StepFunction): boolean[][] {
        const validGrades = isMonotonous(s.map((p) => p[GRADE]));
        const validYield = isMonotonous(s.map((p) => p[YIELD]));
        return validGrades.map((vg, i) => [vg, validYield[i]]);
    }

    $: stepFunctionBounds = computeBounds(stepFunction)
    $: stepFunctionValid = computeValidity(stepFunction)
    $: firstGradeValid = firstGrade >= 0 && firstGrade + OFFSET < stepFunction[0][GRADE]
    $: finalGradeValid = finalGrade + OFFSET >= stepFunction[stepFunction.length - 2][YIELD] && finalGrade <= 1
    $: valid = firstGradeValid && finalGradeValid && stepFunction.every(([a, b]) => a && b)

    function connectLastGrade() {
        const secondToLastIndex = stepFunction.length - 2;
        const lastIndex = stepFunction.length - 1;
        stepFunction[lastIndex][0] = stepFunction[secondToLastIndex][0];
    }

    const ALLOWED_CHARS: Set<String> = new Set<String>("0123456789.")

    function inputGuard(event: KeyboardEvent) {
        if (!ALLOWED_CHARS.has(event.key)) event.preventDefault();
    }


    function isLast(index: number, offset: number = 0): boolean {
        console.error(`${index} ${stepFunction.length -1}`);
        return index === stepFunction.length - 1 - offset;
    }

    function insertPoint(index: number) {
        return () => {
            const newPoints = [...stepFunction];
            const [g1, y1] = stepFunction[index - 1];
            const [g2, y2] = stepFunction[index];
            const p = [0.5 * (g1 + g2), 0.5 * (y1 + y2)]
            newPoints.splice(index, 0, p);
            stepFunction = newPoints;
        }
    }

</script>

<Input type="number" bind:value={firstGrade} class={firstGradeValid ? "": "border-destructive"}></Input>
<Input type="number" bind:value={finalGrade} class={finalGradeValid ? "": "border-destructive"}></Input>
<table>
    <thead>
    <tr>
        <th>Grade [1]</th>
        <th>Yield [1]</th>
        <th/>
    </tr>
    </thead>
    <tbody>
    {#each {length: stepFunction.length - 1} as _, i}
        <tr>
            <td>
                <Input
                        type="number"
                        bind:value={stepFunction[i][GRADE]}
                        on:keypress={inputGuard}
                        class={stepFunctionValid[i][GRADE] ? "": "border-destructive"}
                />
            </td>
            <td><Input
                    type="number"
                    bind:value={stepFunction[i][YIELD]}
                    on:keypress={inputGuard}
                    class={stepFunctionValid[i][YIELD] ? "": "border-destructive"}
            />
            </td>
            <td>
                {#if i > 0}
                    <Button variant="outline" size="icon" on:click={insertPoint(i)}>+</Button>
                {/if}
            </td>
        </tr>
    {/each}
    </tbody>
</table>

<style>
    .insertbutton {
        transform: translateY(50%);
    }
</style>