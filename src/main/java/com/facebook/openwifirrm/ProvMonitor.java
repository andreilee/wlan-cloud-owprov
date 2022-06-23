/*
 * Copyright (c) 2021-present, Facebook, Inc.
 * All rights reserved.
 */

package com.facebook.openwifirrm;

import java.util.concurrent.atomic.AtomicBoolean;
import java.util.Map;
import java.util.Set;
import java.util.TreeSet;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.facebook.openwifirrm.modules.ConfigManager;
import com.facebook.openwifirrm.modules.Modeler;
import com.facebook.openwifirrm.optimizers.ChannelOptimizer;
import com.facebook.openwifirrm.optimizers.MeasurementBasedApApTPC;
import com.facebook.openwifirrm.optimizers.UnmanagedApAwareChannelOptimizer;
import com.facebook.openwifirrm.optimizers.TPC;
import com.facebook.openwifirrm.ucentral.UCentralClient;
import com.facebook.openwifirrm.ucentral.prov.models.VenueList;
import com.facebook.openwifirrm.ucentral.prov.models.Venue;

 /**
 * owprov monitor.
 * Periodically updates owrrm's view of topology using venue information.
 * Also handles periodic optimization, based on owprov configuration.
 */
public class ProvMonitor implements Runnable {
	private static final Logger logger = LoggerFactory.getLogger(ProvMonitor.class);

	/** The main thread reference (i.e. where {@link #run()} is invoked). */
	private Thread mainThread;

	/** The device data manager. */
	private final DeviceDataManager deviceDataManager;

	/** The Modeler module instance. */
	private final Modeler modeler;

	/** The ConfigManager module instance. */
	private final ConfigManager configManager;

	/** The uCentral client. */
	private final UCentralClient client;

	/** Empty constructor without backing files (ex. for unit tests). */
	public ProvMonitor(
		ConfigManager configManager,
		DeviceDataManager deviceDataManager,
		Modeler modeler,
		UCentralClient client
	) {
		this.configManager = configManager;
		this.deviceDataManager = deviceDataManager;
		this.modeler = modeler;
		this.client = client;
	}

	@Override
	public void run() {
		logger.info("Starting ProvMonitor");

		this.mainThread = Thread.currentThread();

		// Run application logic in a periodic loop
		while (!Thread.currentThread().isInterrupted()) {
			try {
				runImpl();
				Thread.sleep(1000/*params.updateIntervalMs*/);
			} catch (InterruptedException e) {
				logger.error("Interrupted!", e);
				break;
			}
		}
		logger.error("Thread terminated!");
	}

	/** Run single iteration of application logic. */
	private void runImpl() {
		if (!client.isInitialized()) {
			logger.trace("Waiting for uCentral client");
			return;
		}
		VenueList venueList = client.getVenues();
		if (venueList == null) {
			logger.error("Venue list request failed");
			return;
		}
		DeviceTopology topo = buildTopology(venueList);
		deviceDataManager.setTopology(topo);
		runOptimizations(deviceDataManager, configManager, modeler);
	}

	/** Build new topology from VenueList */
	protected static DeviceTopology buildTopology(VenueList venueList) {
		// TODO(andreilee): Look at entity hierarchy to understand what has RRM enabled
		DeviceTopology topo = new DeviceTopology();
		for (Venue venue : venueList.venues) {
			String zone = venue.id;
			Set<String> devices = new TreeSet<>(venue.devices);
			topo.put(zone, devices);
		}
		return topo;
	}

	/** Running tx power and channel optimizations for all RRM-enabled venues */
	protected static void runOptimizations(
		DeviceDataManager deviceDataManager,
		ConfigManager configManager,
		Modeler modeler
	) {
		DeviceTopology topo = deviceDataManager.getTopologyCopy();

		for (Map.Entry e : topo.entrySet()) {
			String zone = (String)e.getKey();
			logger.info(
				"Running periodic optimizations\n Zone: {}\n Devices: {}",
				zone,
				e.getValue()
			);
			ChannelOptimizer channelOptimizer = new UnmanagedApAwareChannelOptimizer(
				modeler.getDataModelCopy(), zone, deviceDataManager
			);
			TPC txOptimizer = new MeasurementBasedApApTPC(
				modeler.getDataModelCopy(), zone, deviceDataManager
			);

			Map<String, Map<String, Integer>> channelMap =
				channelOptimizer.computeChannelMap();
			Map<String, Map<String, Integer>> txPowerMap =
				txOptimizer.computeTxPowerMap();

			channelOptimizer.applyConfig(
				deviceDataManager, configManager, channelMap
			);
			txOptimizer.applyConfig(
				deviceDataManager, configManager, txPowerMap
			);
		}
		return;
	}
}
