import GaugeChart from 'react-gauge-chart'

export function MyGauge ({power})  {
    return (
        <>
        <GaugeChart id="gauge-chart5"
                nrOfLevels={420}
                // the intermediate subdivisions correspond to the temperatures of 0, 50 and 100 degrees
                arcsLength={[0.33, 0.33, 0.34]}
                colors={['#5BE12C', '#F5CD19', '#EA4228']}
                percent={power / 3.5}
                arcPadding={0.02}
                textColor={'#333333'}
                needleColor={'#aaaaaa'}
                needleBaseColor={'#aaaaaa'}
                animate={true}
                formatTextValue={value => power+" kW"}
            />
        </>
    )
}
