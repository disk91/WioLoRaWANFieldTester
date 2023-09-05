import { type Request, type Response, Router } from 'express'
import type * as ttn from '../types/ttn.message'
import { getDistance } from 'geolib'
import axios from 'axios'

export interface WioPayload {
  accuracy: number
  altitude: number
  hdop: number
  latitude: number
  longitude: number
  sats: number
  error?: string
}
export interface WioUplink extends ttn.TTNMessage {
  payload: WioPayload
  seq: number
}
export interface WioDownlink {
  sequence_id: number
  rssi_min: number
  rssi_max: number
  distance_min: number
  distance_max: number
  seen: number
}

class WioController {
  public router = Router()

  constructor () {
    this.initRoutes()
  }

  public initRoutes (): void {
    this.router.post('/', this.post)
  }

  public log (msg: string): void {
    const now = (new Date()).toISOString()

    console.log(`[${now}] ${msg}`)
  }

  post = async (req: Request, res: Response): Promise<void> => {
    const wiodata = this.formatRequest(req.body)
    const downlinkUrl = req.header('x-downlink-replace')
    const downlinkKey = req.header('x-downlink-apikey')
    const gateways = wiodata.uplink_message.rx_metadata

    this.log(`Request: ${JSON.stringify(req.body)}`)

    if (wiodata.payload.error !== undefined) {
      this.log(`Error: ${wiodata.payload.error}`)
      res.send(wiodata.payload.error)
      return
    }
    if (downlinkUrl === undefined || downlinkKey === undefined) {
      this.log('Error: missing downlink headers')
      res.send('missing downlink headers')
      return
    }

    const gwRxRssis = gateways.map((gateway) => gateway.rssi)
    const gwDistances = gateways.filter((gateway) => {
      return gateway.location !== undefined
    }).map((gateway) => getDistance(wiodata.payload, gateway.location))

    const downlink: WioDownlink = {
      sequence_id: wiodata.seq % 255,
      rssi_min: (gwRxRssis.length > 0) ? Math.min(...gwRxRssis) : 0,
      rssi_max: (gwRxRssis.length > 0) ? Math.max(...gwRxRssis) : 0,
      distance_min: (gwDistances.length > 0) ? Math.min(...gwDistances) : 0,
      distance_max: (gwDistances.length > 0) ? Math.max(...gwDistances) : 0,
      seen: gateways.length
    }
    const packed = this.packDownlink(downlink)
    this.log(`Downlink: ${JSON.stringify(downlink)} => ${packed.toString('hex').toUpperCase()}`)

    // Send results back to Wio terminal
    const payload = {
      downlinks: [
        {
          frm_payload: packed.toString('base64'),
          f_port: 2,
          priority: 'NORMAL'
        }
      ]
    }
    try {
      await axios.post(
        downlinkUrl,
        payload,
        {
          headers: {
            Authorization: `Bearer ${downlinkKey}`,
            'Content-Type': 'application/json'
          }
        }
      )
    } catch (error) {
      console.log(error)
    }

    res.send('OK')
  }

  private formatRequest (request: ttn.TTNMessage): WioUplink {
    const bytes = Buffer.from(request.uplink_message.frm_payload, 'base64')
    let decoded = {} as WioPayload

    if (request.uplink_message.f_port === 1) {
      const lonSign = (bytes[0] >> 7) & 0x01 ? -1 : 1
      const latSign = (bytes[0] >> 6) & 0x01 ? -1 : 1

      const encLat = ((bytes[0] & 0x3f) << 17) +
                (bytes[1] << 9) +
                (bytes[2] << 1) +
                (bytes[3] >> 7)

      const encLon = ((bytes[3] & 0x7f) << 16) +
                (bytes[4] << 8) +
                bytes[5]

      const hdop = bytes[8] / 10
      const sats = bytes[9]

      const maxHdop = 2
      const minSats = 5

      if ((hdop < maxHdop) && (sats >= minSats)) {
        // Send only acceptable quality of position to mappers
        decoded.latitude = latSign * (encLat * 108 + 53) / 10000000
        decoded.longitude = lonSign * (encLon * 215 + 107) / 10000000
        decoded.altitude = ((bytes[6] << 8) + bytes[7]) - 1000
        decoded.accuracy = (hdop * 5 + 5) / 10
        decoded.hdop = hdop
        decoded.sats = sats
      } else {
        decoded.error = `Need more GPS precision (hdop must be <${maxHdop} & sats must be >= ${minSats}) current hdop: ${hdop} & sats: ${sats}`
      }
    } else {
      decoded.error = 'unknown port'
    }

    return {
      ...request,
      payload: decoded,
      seq: request.uplink_message.f_cnt ?? 0
    }
  }

  private packDownlink (downlink: WioDownlink): Buffer {
    const bytes = Buffer.alloc(6)
    bytes[0] = downlink.sequence_id
    bytes[1] = downlink.rssi_min + 200
    bytes[2] = downlink.rssi_max + 200
    if (downlink.distance_min === 0) {
      bytes[3] = 0
    } else {
      bytes[3] = Math.min(128, (downlink.distance_min / 250) + 1)
    }
    if (downlink.distance_max === 0) {
      bytes[4] = 0
    } else {
      bytes[4] = Math.min(128, (downlink.distance_max / 250) + 1)
    }
    bytes[5] = downlink.seen

    return bytes
  }
}

export default WioController
