<script lang="ts">

    import MainModuleFactory, {type MainModule, type PointVector} from "../static/core";
    import {Wave} from "svelte-loading-spinners";
    import StepFunctionTable from "./StepFunctionTable.svelte";
    import {writable, type Writable} from "svelte/store";
    import Chart from "$lib/Chart.svelte";
    import {Input} from "$lib/components/ui/input/index.js";

    type CoreModule = MainModule;
    type StepFunctionPoints = [[number, number]];

    export const stepFunctionPoints = writable([[0.25, 0.0],
        [0.5, 0.5],
        [0.75, 0.75],
        [0.75, 1.0]]);

    let firstGrade = 0.125;
    let finalGrade = 0.925;

    let stepFunctionValid = writable(true);

</script>

{#await MainModuleFactory()}
    <Wave duration="1s"></Wave>
{:then coreModule}
    <StepFunctionTable coreModule={coreModule}></StepFunctionTable>
    <Chart coreModule={coreModule} stepFunction={stepFunctionPoints} firstGrade={firstGrade}
           finalGrade={finalGrade} show={$stepFunctionValid}></Chart>
{/await}