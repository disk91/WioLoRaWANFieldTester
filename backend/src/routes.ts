import { Router } from 'express'
import WioController from './controller/WioLoRaWANFieldTester'

const routes = Router()

const wio = new WioController()
routes.use('/fieldtester/ttn/v3', wio.router)

export default routes
