<script lang="ts">
    import {LineChart, Line, Layer, Chart as LayerChart} from 'layerchart';
    import {onMount} from 'svelte';
    import { Button } from '$lib/components/ui/button/index.js';
    import * as Chart from '$lib/components/ui/chart/index.js';
    import * as Card from '$lib/components/ui/card/index.js';
    import * as Optimizer from '$lib/wasm/core.js';

    const numPoints = 250;
    let {stepFunction, firstGrade, finalGrade} = $props();
    let optimizer = $state(null)
    let optimized = $state<boolean>(false);
    const chartConfig = {} satisfies Chart.ChartConfig;

    onMount(async () => {
        optimizer = await Optimizer.default();
    });

    type ChartRow = {
        grade: number;
        yieldStep?: number;
        yieldInitial?: number;
    }

    const stepFunctionPoints = $derived.by(() => {
        if (!stepFunction) return undefined;
        let sf = stepFunction.map((p) => {
            return {
                grade: p.grade / 100,
                yieldStep: p.yield / 100
            };
        });
        sf = [{grade: 0, yieldStep: 0}, ...sf, {grade: 1, yieldStep: 1}];
        const points: ChartRow[] = [];

        points.push({grade: 0, yieldStep: 0});
        for (let i = 1; i < sf.length - 1; i++) {
            const {grade: g, yieldStep: y} = sf[i];
            points.push({grade: sf[i - 1].grade, yieldStep: y});
            points.push({grade: g, yieldStep: y});
            points.push({grade: g, yieldStep: sf[i + 1].yieldStep});
        }
        points.push({grade: 1, yieldStep: 1});
        return points as ChartRow[];
    });
    const intergrowth = $derived.by(() => {
        if (stepFunctionPoints && firstGrade != null && finalGrade != null && optimizer) {
            const points = new optimizer.PointVector();
            stepFunction.forEach((p) => points.push_back([p.grade / 100, p.yield / 100]));
            optimizer = false;
            return optimizer.Intergrowth.fromStepFunction(points, firstGrade / 100, finalGrade / 100);
        }
        return undefined;
    });

    function splineData(spline, xaxis?): { grade: number, yield: number }[] {
        let xAxis = xaxis;
        if (!xAxis) {
            const [minX, maxX] = spline.bounds();
            xAxis = optimizer.linspace(minX, maxX, numPoints);
        }
        const line = spline.evaluateFast(xAxis).toArray();
        return xAxis.toArray().map((g, index) => {
            const y = line[index];
            return {
                grade: g, yield: y
            };
        }).filter((row) => !isNaN(row.yield) && !isNaN(row.grade));
    }

    const initialSplinePoints = $derived.by(() => {
        if (intergrowth) {

            return splineData(intergrowth.initial).map((row) => {
                return {
                    grade: row.grade,
                    yieldInitial: row.yield,
                } as ChartRow
            });
        }

        return [];
    });

    $inspect('points', stepFunctionPoints);
    $inspect('fristGrade', firstGrade);
    $inspect('finalGrade', finalGrade);
    $inspect('intergrowth', intergrowth);
    $inspect('points', initialSplinePoints);

</script>

<Card.Root style="width: 500px; height: 200px">
    <Card.Header>
        <Card.Title>Line Chart - Linear</Card.Title>
        <Card.Description>Showing total visitors for the last 6 months</Card.Description>
    </Card.Header>
    <Card.Content>
        <Chart.Container config={chartConfig} class="max-h-[200px] w-full">
            <LineChart
                    yDomain={[1, 0]}
                    xDomain={[0, 1]}
                    x="grade"
                    series={[
                        {
                           key: "yieldStep",
                            data: stepFunctionPoints,
                            color: "#ff0000",
                            label: "Step Functions",
                        },
                        {
                           key: "yieldInitial",
                            data: initialSplinePoints,
                            color: "#ff0000",
                            label: "Step Functions",
                        }
                    ]}
            >

            </LineChart>
        </Chart.Container>
    </Card.Content>
    <Card.Footer>
        <Button></Button>
    </Card.Footer>
</Card.Root>
