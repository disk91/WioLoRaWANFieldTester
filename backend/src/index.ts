import express, { type Express } from 'express'
import logger from 'morgan'
import routes from './routes'
import dotenv from 'dotenv'

dotenv.config()

const app: Express = express()
const port = process.env.PORT!

app.use(logger('combined'))
app.use(express.json())

app.use('/', routes)

app.listen(port, () => {
  console.log(`⚡️[server]: Server is running at http://localhost:${port}`)
})
