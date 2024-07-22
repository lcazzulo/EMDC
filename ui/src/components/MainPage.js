import React, { useState, useEffect, useMemo } from 'react';
import { socket } from '../socket';
import { MyGauge }  from './MyGauge';

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
            //console.log(value)
            var value_json = JSON.parse(value);
            set_active_power(format_three_decimal_places(value_json["power"]));

        }

        socket.on('connect', onConnect);
        socket.on('disconnect', onDisconnect);
        socket.on('POWER.ACTIVE', onPowerActive);

        return () => {
            socket.off('connect', onConnect);
            socket.off('disconnect', onDisconnect);
            socket.off('POWER.ACTIVE', onPowerActive);
        };
    }, []);


    return (
        <div>
            <h2>MainPage</h2>
            <MyGauge key="gauge-active-power" power={active_power} />
        </div>
    )
}

export default MainPage;
