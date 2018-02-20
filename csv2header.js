/* RLE compression for a byte array of level data */

const fs = require('fs');
const _ = require('lodash');
const path = require('path');

const args = process.argv.slice(2);
const filePath = path.resolve(process.cwd(), args[0]);
const parsedPath = path.parse(filePath);

const mapName = parsedPath.name;
if ( !mapName ) {
	console.error('Error: no map name specified');
	process.exit(1);
}

const MAX_COL_LENGTH = 40;
const filename = path.resolve(`${filePath}.csv`);
const fileContents = fs.readFileSync(filename);
const fileString = fileContents.toString();
const fileArray = fileString.split('\n');
let fileData = [];

let rowLength;
let rowCount = 0;


if ( fileArray[0] ) {
	const lineData = fileArray[0].split(',');
	rowLength = lineData.length;
}


_.each(fileArray, (line, index) => {
	if ( line.length ) {
		fileArray[index] += ',';
		rowCount++;
	}
});

const arraySize = rowLength * rowCount;
const joinedRows = _.trimEnd(fileArray.join('\n'), '\n\r\t \,');

//console.log('array size', arraySize);
//console.log('data rows', fileArray);

let output = `const uint8_t ${mapName}_coll[${arraySize}] = {\n`;
output += joinedRows;
output += '\n};\n';

const collFile = path.resolve(`${filePath}_coll.h`);
fs.writeFileSync(collFile, output);

console.log(`Wrote collision map to ${collFile}`);
