import ReactSpeedometer from "react-d3-speedometer"



export function MyGauge ({power})  {
    return (
        <>
		<ReactSpeedometer
  			maxValue={3.6}
  			value={power}
  			needleColor="red"
  			startColor="green"
  			segments={10}
  			endColor="red"
                        fluidWidth="true"
		/>
        </>
    )
}
