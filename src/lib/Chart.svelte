<script lang="ts">
    import {derived, type Readable, readable, type Writable} from "svelte/store";
    import type {Intergrowth, MainModule, PointVector} from "../static/core";
    import * as d3 from 'd3';
    import {onMount} from "svelte";
    import {draw, fade} from 'svelte/transition';
    import {symbolPlus} from "d3";

    type Point = [number, number];
    type CoreModule = MainModule;
    export let stepFunction: [Point];
    export let coreModule: CoreModule;
    let data: number[] = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];
    export let width: number = 640;
    export let height: number = 400;
    export let marginTop: number = 20;
    export let marginRight: number = 20;
    export let marginBottom: number = 20;
    export let marginLeft: number = 20;

    export let numPoints: number = 150;

    export let firstGrade: number;
    export let finalGrade: number;
    let showOptimized: boolean = true;


    function convertStepFunction(s: [Point]): PointVector {
        let vector = new coreModule.PointVector();
        for (const point of s) vector.push_back(point);
        return vector;
    }

    function computeStepFunctionPoints(pts: [Point]): number[][] {
        const points = [[0, pts[0][1]]];
        for (let i = 0; i < pts.length - 1; i++) {
            const [g1, y1] = pts[i];
            const [_, y2] = pts[i + 1];
            points.push([g1, y1]);
            points.push([g1, y2]);
        }
        points.push(pts[pts.length - 1]);
        points.push([1.0, 1.0]);
        return points;
    }

    $: stepFunctionCore = convertStepFunction(stepFunction)
    $: intergrowthFunction = coreModule.Intergrowth.fromStepFunction(stepFunctionCore, firstGrade, finalGrade)!

    $: spline = showOptimized ? intergrowthFunction.optimized() : intergrowthFunction.initial;
    $: breakPoints = spline?.breakPoints().toArray();
    $: xSpline = coreModule.linspace(spline?.bounds()[0], spline?.bounds()[1], numPoints);
    $: ySpline = spline.evaluateFast(xSpline);
    $: splinePoints = xSpline.toArray().map((_x: number, i: number ) => [x(_x), y(ySpline.get(i!))]);

    $: stepFunctionPoints = computeStepFunctionPoints(stepFunction).map((p) => [x(p[0]), y(p[1])]);


    $: x = d3.scaleLinear([0, 1], [marginLeft, width - marginRight]);
    $: y = d3.scaleLinear([1, 0], [height - marginBottom, marginTop]);
    $: line = d3.line<number>().x((d) => x(d)).y((_, i) => y(ySpline.get(i)));

    let gx: any;
    let gy: any;
    $: d3.select(gy).call(d3.axisLeft(y));
    $: d3.select(gx).call(d3.axisBottom(x));

    let show = false;
    onMount(() => show = true);
</script>

<!-- Add chart -->
<svg width={width} height={height}>
    <!-- Add x-axis -->
    <g bind:this={gx} transform="translate(0,{height - marginBottom})" color="black"/>
    <!-- Add y-axis -->
    <g bind:this={gy} transform="translate({marginLeft},0)" color="black"/>
    <!-- Add line -->
    {#if show}
        <path stroke-width="1.5" d={d3.line()(splinePoints)}
              stroke="black" fill="none"/>
    {/if}
    {#if show}
        <path stroke-width="1" d={d3.line()(stepFunctionPoints)}
              stroke="steelblue" fill="none"/>
    {/if}
    <!-- Add data points -->
   <!-- <g stroke-width="1.5">
        {#if show}
            {#each breakPoints as [_grade, _yield], i}
                <circle cx={x(_grade)} cy={y(_yield)} r="2.5" fill="black"/>
            {/each}
        {/if}
    </g>-->
</svg>