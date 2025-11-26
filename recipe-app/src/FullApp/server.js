import path from 'path';
import express from 'express';
import { fileURLToPath } from 'url';
import { spawn } from 'child_process';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const app = express();

app.use(express.static(__dirname));

app.get('/', (req, res) => {
  res.sendFile(path.join(__dirname, 'home.html'));
});

const port = process.env.port || 3000;
app.listen(port, () => {
  console.log(`Listening on http://127.0.0.1:${port}`);
});