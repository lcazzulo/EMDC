import React, { useState, useEffect, useMemo } from 'react';
import { socket } from '../socket';
import { MyGauge }  from './MyGauge';
import Nbsp  from './nbsp';

const MainPage = () => {

    const [isConnected, setIsConnected] = useState(socket.connected);
    const [active_power, set_active_power] = useState(0.0);
    const [reactive_power, set_reactive_power] = useState(0.0);

    function format_three_decimal_places (num)
    {
        return (Math.round(num * 1000) / 1000).toFixed(3);
    }

    useEffect(() => {

        console.log("useEffect")


        socket.timeout(5000).emit('join', '')

        function onConnect() {
            setIsConnected(true);
            console.log('join')
            socket.timeout(5000).emit('join', '')
        }

        function onDisconnect() {
            setIsConnected(false);
        }



        function onPowerActive(value) {
            var value_json = JSON.parse(value);
	    var new_val = format_three_decimal_places(value_json["power"]);
	    if (new_val !== active_power)
	    {
            	set_active_power(new_val);
            }

        }

	function onPowerReactive(value) {
            var value_json = JSON.parse(value);
            var new_val = format_three_decimal_places(value_json["power"]);
	    if (new_val !== reactive_power)
            {
            	set_reactive_power(new_val);
	    }

        }

        socket.on('connect', onConnect);
        socket.on('disconnect', onDisconnect);
        socket.on('POWER.ACTIVE', onPowerActive);
	socket.on('POWER.REACTIVE', onPowerReactive);

        return () => {
            socket.off('connect', onConnect);
            socket.off('disconnect', onDisconnect);
            socket.off('POWER.ACTIVE', onPowerActive);
	    socket.on('POWER.REACTIVE', onPowerReactive);
        };
    }, []);


    return (
        <div>
            <table style={{width: '98%'}}>
		<thead>
			<tr>
				<th scope="col" style={{textAlign: 'center', backgroundImage: 'linear-gradient(to right, red, snow)'}}>Active power<Nbsp /><Nbsp /></th>
				<th scope="col" style={{textAlign: 'center', backgroundImage: 'linear-gradient(to right, blue, snow)'}}>Reactive power</th>
			</tr>
		</thead>
		<tbody>
			<tr className="table-light">
				<td style={{width: '50%', height: '500px', backgroundColor: '#f7d9d7'}}><MyGauge key="gauge-active-power" power={active_power} /></td>
				<td style={{width: '50%', height: '500px', backgroundColor: '#e8e6ff'}}><MyGauge key="gauge-reactive-power" power={reactive_power} /></td>
			</tr>
		</tbody>
            </table>
        </div>
    )
}

export default MainPage;
