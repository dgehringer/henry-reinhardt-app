<script lang="ts">
    import {MoreVertical, ListStart, ListEnd, Minus, X, Check} from '@lucide/svelte';
    import {Input} from '$lib/components/ui/input/index.js';
    import {Button} from '$lib/components/ui/button/index.js';
    import * as Resizable from "$lib/components/ui/resizable/index.js";
    import * as DropdownMenu from '$lib/components/ui/dropdown-menu/index.js';
    import * as ButtonGroup from '$lib/components/ui/button-group/index.js';
    import * as InputGroup from '$lib/components/ui/input-group/index.js';
    import * as Tooltip from '$lib/components/ui/tooltip/index.js';
    import HRChart from './HRChart.svelte';

    type PointValidationResult = {
        grade: string | null;
        yield: string | null;
    };

    const EPS = 1e-3;
    let dropdownOpenIndex = $state(null);
    let firstGrade = $state(10);
    let finalGrade = $state(80);
    let stepFunction = $state([
        {grade: 40, yield: 60},
        {grade: 60, yield: 80}
    ]);
    const validation = $derived.by(() => {
        const results: PointValidationResult[] = [];
        let prevGrade = firstGrade;
        let prevYield = 0;

        for (const {grade: g, yield: y} of stepFunction) {
            const validation: PointValidationResult = {
                grade: null,
                yield: null
            };

            if (g === null) {
                validation.grade = 'Grade is not a valid number';
            }
            if (y === null) {
                validation.yield = 'Yield is not a valid number';
            }
            if (g <= prevGrade + EPS && !validation.grade) {
                validation.grade = `Grades must be strictly increasing: previous grade was ${prevGrade}, current grade is ${g}`;
            }
            if (y <= prevYield + EPS && !validation.yield) {
                validation.yield = `Yields must be strictly increasing: previous yield was ${prevYield}, current yield is ${y}`;
            }
            prevGrade = g;
            prevYield = y;
            results.push(validation);
        }
        results.push({
            grade: finalGrade <= prevGrade + EPS ? 'Grades must be strictly increasing' : null,
            yield: null
        });

        return results;
    });
    const validStepFunction = $derived.by(() => {
        if (validation.some((v) => v.grade !== null || v.yield !== null)) {
            return null;
        }
        return stepFunction;
    });

    function removePoint(index: number) {
        stepFunction = stepFunction.filter((_, i) => i !== index);
    }

    function insertPoint(between: number, and: number) {
        if (Math.abs(and - between) !== 1) {
            throw new Error('Can only insert between two adjacent points');
        }

        const getPoint = (i: number) => {
            if (i < 0) return {grade: firstGrade, yield: 0};
            if (i >= stepFunction.length) return {grade: finalGrade, yield: 100};
            return stepFunction[i];
        };

        const a = getPoint(between);
        const b = getPoint(and);

        stepFunction = [
            ...stepFunction.slice(0, between),
            {
                grade: (a.grade + b.grade) / 2,
                yield: (a.yield + b.yield) / 2
            },
            ...stepFunction.slice(between)
        ];
    }

    function closeDropdownAnd(fn) {
        return () => {
            fn();
            dropdownOpenIndex = null;
        };
    }
</script>

<Resizable.PaneGroup direction="vertical">
    <Resizable.Pane>
        <div class="flex w-full max-w-sm items-center gap-2">
            <Input type="number" placeholder="Grade" bind:value={firstGrade}/>
            <Input type="number" placeholder="Yield" disabled value="0"/>
        </div>
        {#each stepFunction as step, index}
            <div class="flex w-full max-w-sm items-center gap-2">
                <InputGroup.Root>
                    <InputGroup.Input type="number" placeholder="Grade" bind:value={step.grade}/>
                </InputGroup.Root>
                <InputGroup.Root>
                    <InputGroup.Input type="number" placeholder="Yield" bind:value={step.yield}/>
                    <InputGroup.Addon align="inline-end">
                        {#if validation[index].yield !== null}
                            <Tooltip.Provider>
                                <Tooltip.Root>
                                    <Tooltip.Trigger>
                                        <X class="w-4"/>
                                    </Tooltip.Trigger>
                                    <Tooltip.Content side="top" class="max-w-xs">
                                        {validation[index].yield}
                                        <Tooltip.Arrow/>
                                    </Tooltip.Content>
                                </Tooltip.Root>
                            </Tooltip.Provider>
                        {:else}
                            <Check class="w-3"/>
                        {/if}
                    </InputGroup.Addon>
                </InputGroup.Root>
                <DropdownMenu.Root
                        open={dropdownOpenIndex === index}
                        onOpenChange={(v) => (dropdownOpenIndex = v ? index : null)}
                >
                    <DropdownMenu.Trigger>
                        <Button variant="outline" size="icon" aria-label="Submit">
                            <MoreVertical/>
                        </Button>
                    </DropdownMenu.Trigger>
                    <DropdownMenu.Content side="right" class="w-fit max-w-fit min-w-fit">
                        <ButtonGroup.Root orientation="vertical" aria-label="Controls" class="h-fit">
                            <Button
                                    variant="outline"
                                    size="icon"
                                    onclick={closeDropdownAnd(() => insertPoint(index, index - 1))}
                            >
                                <ListStart/>
                            </Button>
                            {#if stepFunction.length > 1}
                                <Button
                                        variant="outline"
                                        size="icon"
                                        onclick={closeDropdownAnd(() => removePoint(index))}
                                >
                                    <Minus/>
                                </Button>
                            {/if}
                            <Button
                                    variant="outline"
                                    size="icon"
                                    onclick={closeDropdownAnd(() => insertPoint(index + 1, index))}
                            >
                                <ListEnd/>
                            </Button>
                        </ButtonGroup.Root>
                    </DropdownMenu.Content>
                </DropdownMenu.Root>
            </div>
        {/each}
        <div class="flex w-full max-w-sm items-center gap-2">
            <Input type="number" placeholder="Grade" bind:value={finalGrade}/>
            <Input type="number" placeholder="Yield" disabled value="100"/>
        </div>
    </Resizable.Pane>
    <Resizable.Handle/>
    <Resizable.Pane>
        <HRChart stepFunction={validStepFunction} {firstGrade} {finalGrade}/>
    </Resizable.Pane>
</Resizable.PaneGroup>