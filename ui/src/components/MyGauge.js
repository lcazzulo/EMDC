import GaugeComponent from 'react-gauge-component'
import {format_three_decimal_places} from '../utils.js'

export function MyGauge ({power})  {

     var stringLabel = format_three_decimal_places(power) + ' kW';
     return (
        <>
		<GaugeComponent
			value={power}
  			minValue={0}
  			maxValue={3.6}
                        arc={{
                            subArcs: [ { limit: 1.2, color: "#5BE12C"}, { limit: 2.4, color: "#F5CD19"}, { color: "#EA4228"}, ]
                        }}
                        labels={{
    				valueLabel: { formatTextValue: value => stringLabel, maxDecimalDigits: 3, style: {fontSize: "40px", fill: "#cccccc"} },
                                tickLabels: 
				{
                                	defaultTickValueConfig:
					{
						style: {fontSize: "20px"}
					}
				}
    			}}
		/>
        </>
    )
}
