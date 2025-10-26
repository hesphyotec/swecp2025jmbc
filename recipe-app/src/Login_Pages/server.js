import path from 'path';
import express from 'express';
import { fileURLToPath } from 'url';
import { spawn } from 'child_process';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const app = express();

const db = new sqlite3.Database((path.join(__dirname, "BRHitting.sqlite")), sqlite3.OPEN_READONLY, (err) => {
    if (err){
        console.error("Error: ", err.message);
    }
    else{
        console.log("Opened DB")
    }
});
const executeCPP = async (script, args) =>{
  const cpp = spawn("cpp", [script, args]);

  const result = await new Promise((res, rej) => {
    let output;

    cpp.stdout.on('data', (data) =>{
        output = data;
    });

    cpp.stderr.on('data', (data) =>{
        console.error("Error", data.toString());
        rej("Error Ocurred");
    });

    cpp.on("exit", (code) =>{
      console.log("Exited with code ", code);
      res(output);
    });
  });
  return result;
}


app.use(express.static(path.join(__dirname)));

app.get('/', (req, res) => {
  res.sendFile(path.join(__dirname, 'home.html'));
});

app.listen(3000, '127.0.0.1', () => {
  console.log('Listening on http://127.0.0.1:3000');
});