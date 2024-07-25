export function format_three_decimal_places (num)
{
	return (Math.round(num * 1000) / 1000).toFixed(3);
}

